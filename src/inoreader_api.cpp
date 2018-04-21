#include "inoreader_api.h"

#include <cstring>
#include <curl/curl.h>
#include <json.h>
#include <vector>

#include "config.h"
#include "strprintf.h"
#include "utils.h"

#define INOREADER_LOGIN "https://inoreader.com/accounts/ClientLogin"
#define INOREADER_API_PREFIX "https://inoreader.com/reader/api/0/"
#define INOREADER_FEED_PREFIX "https://inoreader.com/reader/atom/"

#define INOREADER_SUBSCRIPTION_LIST INOREADER_API_PREFIX "subscription/list"
#define INOREADER_API_MARK_ALL_READ_URL INOREADER_API_PREFIX "mark-all-as-read"
#define INOREADER_API_EDIT_TAG_URL INOREADER_API_PREFIX "edit-tag"
#define INOREADER_API_TOKEN_URL INOREADER_API_PREFIX "token"

#define INOREADER_APP_ID "AppId: 1000000394"
#define INOREADER_APP_KEY "AppKey: CWcdJdSDcuxHYoqGa3RsPh7X2DZ2MmO7"

// for reference, see https://inoreader.com/developers

namespace newsboat {

inoreader_api::inoreader_api(configcontainer* c)
	: remote_api(c)
{
	// TODO
}

inoreader_api::~inoreader_api()
{
	// TODO
}

bool inoreader_api::authenticate()
{
	auth = retrieve_auth();
	LOG(level::DEBUG, "inoreader_api::authenticate: Auth = %s", auth);
	return auth != "";
}

static size_t
my_write_data(void* buffer, size_t size, size_t nmemb, void* userp)
{
	std::string* pbuf = static_cast<std::string*>(userp);
	pbuf->append(static_cast<const char*>(buffer), size * nmemb);
	return size * nmemb;
}

std::string inoreader_api::retrieve_auth()
{
	CURL* handle = curl_easy_init();
	credentials cred = get_credentials("inoreader", "Inoreader");
	if (cred.user.empty() || cred.pass.empty()) {
		return "";
	}

	char* username = curl_easy_escape(handle, cred.user.c_str(), 0);
	char* password = curl_easy_escape(handle, cred.pass.c_str(), 0);

	std::string postcontent =
		strprintf::fmt("Email=%s&Passwd=%s", username, password);

	curl_free(username);
	curl_free(password);

	std::string result;

	curl_slist* list = NULL;
	list = curl_slist_append(list, INOREADER_APP_ID);
	list = curl_slist_append(list, INOREADER_APP_KEY);

	utils::set_common_curl_options(handle, cfg);
	curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, my_write_data);
	curl_easy_setopt(handle, CURLOPT_WRITEDATA, &result);
	curl_easy_setopt(handle, CURLOPT_POSTFIELDS, postcontent.c_str());
	curl_easy_setopt(handle, CURLOPT_URL, INOREADER_LOGIN);
	curl_easy_perform(handle);
	curl_easy_cleanup(handle);
	curl_slist_free_all(list);

	std::vector<std::string> lines = utils::tokenize(result);
	for (const auto& line : lines) {
		LOG(level::DEBUG,
		    "inoreader_api::retrieve_auth: line = %s",
		    line);
		const std::string auth_string = "Auth=";
		if (line.compare(0, auth_string.length(), auth_string) == 0) {
			std::string auth = line.substr(auth_string.length());
			return auth;
		}
	}

	return "";
}

std::vector<tagged_feedurl> inoreader_api::get_subscribed_urls()
{
	std::vector<tagged_feedurl> urls;
	curl_slist* custom_headers{};

	CURL* handle = curl_easy_init();
	std::string result;
	add_custom_headers(&custom_headers);
	curl_easy_setopt(handle, CURLOPT_HTTPHEADER, custom_headers);

	utils::set_common_curl_options(handle, cfg);
	curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, my_write_data);
	curl_easy_setopt(handle, CURLOPT_WRITEDATA, &result);
	curl_easy_setopt(handle, CURLOPT_URL, INOREADER_SUBSCRIPTION_LIST);
	curl_easy_perform(handle);
	curl_easy_cleanup(handle);
	curl_slist_free_all(custom_headers);

	LOG(level::DEBUG,
	    "inoreader_api::get_subscribed_urls: document = %s",
	    result);

	json_object* reply = json_tokener_parse(result.c_str());
	if (reply == nullptr) {
		LOG(level::ERROR,
		    "inoreader_api::get_subscribed_urls: failed to parse "
		    "response as JSON.");
		return urls;
	}

	json_object* subscription_obj{};
	json_object_object_get_ex(reply, "subscriptions", &subscription_obj);
	struct array_list* subscriptions =
		json_object_get_array(subscription_obj);

	int len = array_list_length(subscriptions);

	for (int i = 0; i < len; i++) {
		std::vector<std::string> tags;
		json_object* sub =
			json_object_array_get_idx(subscription_obj, i);

		json_object* node{};

		json_object_object_get_ex(sub, "id", &node);
		const char* id = json_object_get_string(node);

		json_object_object_get_ex(sub, "title", &node);
		const char* title = json_object_get_string(node);

		tags.push_back(std::string("~") + title);

		json_object_object_get_ex(sub, "categories", &node);
		struct array_list* categories = json_object_get_array(node);

#if JSON_C_MAJOR_VERSION == 0 && JSON_C_MINOR_VERSION < 13
		for (int i = 0; i < array_list_length(categories); i++) {
#else
		for (size_t i = 0; i < array_list_length(categories); i++) {
#endif
			json_object* cat = json_object_array_get_idx(node, i);
			json_object* label_node{};
			json_object_object_get_ex(cat, "label", &label_node);
			const char* label = json_object_get_string(label_node);
			tags.push_back(std::string(label));
		}

		auto url = strprintf::fmt(
			"%s%s?n=%u",
			INOREADER_FEED_PREFIX,
			id,
			cfg->get_configvalue_as_int("inoreader-min-items"));
		urls.push_back(tagged_feedurl(url, tags));
	}

	json_object_put(reply);

	return urls;
}

void inoreader_api::add_custom_headers(curl_slist** custom_headers)
{
	if (auth_header.empty()) {
		auth_header = strprintf::fmt(
			"Authorization: GoogleLogin auth=%s", auth);
	}
	LOG(level::DEBUG,
	    "inoreader_api::add_custom_headers header = %s",
	    auth_header);
	*custom_headers =
		curl_slist_append(*custom_headers, auth_header.c_str());
	*custom_headers = curl_slist_append(*custom_headers, INOREADER_APP_ID);
	*custom_headers = curl_slist_append(*custom_headers, INOREADER_APP_KEY);
}

bool inoreader_api::mark_all_read(const std::string& feedurl)
{
	std::string real_feedurl =
		feedurl.substr(strlen(INOREADER_FEED_PREFIX));
	std::vector<std::string> elems = utils::tokenize(real_feedurl, "?");
	real_feedurl = utils::unescape_url(elems[0]);
	std::string token = get_new_token();

	std::string postcontent =
		strprintf::fmt("s=%s&T=%s", real_feedurl, token);

	std::string result =
		post_content(INOREADER_API_MARK_ALL_READ_URL, postcontent);

	return result == "OK";
}

bool inoreader_api::mark_article_read(const std::string& guid, bool read)
{
	std::string token = get_new_token();
	return mark_article_read_with_token(guid, read, token);
}

bool inoreader_api::mark_article_read_with_token(
	const std::string& guid,
	bool read,
	const std::string& token)
{
	std::string postcontent;

	if (read) {
		postcontent = strprintf::fmt(
			"i=%s&a=user/-/state/com.google/read&r=user/-/state/"
			"com.google/kept-unread&ac=edit&T=%s",
			guid,
			token);
	} else {
		postcontent = strprintf::fmt(
			"i=%s&r=user/-/state/com.google/read&a=user/-/state/"
			"com.google/kept-unread&a=user/-/state/com.google/"
			"tracking-kept-unread&ac=edit&T=%s",
			guid,
			token);
	}

	std::string result =
		post_content(INOREADER_API_EDIT_TAG_URL, postcontent);

	LOG(level::DEBUG,
	    "inoreader_api::mark_article_read_with_token: postcontent = %s "
	    "result = %s",
	    postcontent,
	    result);

	return result == "OK";
}

std::string inoreader_api::get_new_token()
{
	CURL* handle = curl_easy_init();
	std::string result;
	curl_slist* custom_headers{};

	utils::set_common_curl_options(handle, cfg);
	add_custom_headers(&custom_headers);
	curl_easy_setopt(handle, CURLOPT_HTTPHEADER, custom_headers);
	curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, my_write_data);
	curl_easy_setopt(handle, CURLOPT_WRITEDATA, &result);
	curl_easy_setopt(handle, CURLOPT_URL, INOREADER_API_TOKEN_URL);
	curl_easy_perform(handle);
	curl_easy_cleanup(handle);
	curl_slist_free_all(custom_headers);

	LOG(level::DEBUG, "inoreader_api::get_new_token: token = %s", result);

	return result;
}

bool inoreader_api::update_article_flags(
	const std::string& inoflags,
	const std::string& newflags,
	const std::string& guid)
{
	std::string star_flag = cfg->get_configvalue("inoreader-flag-star");
	std::string share_flag = cfg->get_configvalue("inoreader-flag-share");
	bool success = true;

	if (star_flag.length() > 0) {
		if (strchr(inoflags.c_str(), star_flag[0]) == nullptr
		    && strchr(newflags.c_str(), star_flag[0]) != nullptr) {
			success = star_article(guid, true);
		} else if (
			strchr(inoflags.c_str(), star_flag[0]) != nullptr
			&& strchr(newflags.c_str(), star_flag[0]) == nullptr) {
			success = star_article(guid, false);
		}
	}

	if (share_flag.length() > 0) {
		if (strchr(inoflags.c_str(), share_flag[0]) == nullptr
		    && strchr(newflags.c_str(), share_flag[0]) != nullptr) {
			success = share_article(guid, true);
		} else if (
			strchr(inoflags.c_str(), share_flag[0]) != nullptr
			&& strchr(newflags.c_str(), share_flag[0]) == nullptr) {
			success = share_article(guid, false);
		}
	}

	return success;
}

bool inoreader_api::star_article(const std::string& guid, bool star)
{
	std::string token = get_new_token();
	std::string postcontent;

	if (star) {
		postcontent = strprintf::fmt(
			"i=%s&a=user/-/state/com.google/starred&ac=edit&T=%s",
			guid,
			token);
	} else {
		postcontent = strprintf::fmt(
			"i=%s&r=user/-/state/com.google/starred&ac=edit&T=%s",
			guid,
			token);
	}

	std::string result =
		post_content(INOREADER_API_EDIT_TAG_URL, postcontent);

	return result == "OK";
}

bool inoreader_api::share_article(const std::string& guid, bool share)
{
	std::string token = get_new_token();
	std::string postcontent;

	if (share) {
		postcontent = strprintf::fmt(
			"i=%s&a=user/-/state/com.google/broadcast&ac=edit&T=%s",
			guid,
			token);
	} else {
		postcontent = strprintf::fmt(
			"i=%s&r=user/-/state/com.google/broadcast&ac=edit&T=%s",
			guid,
			token);
	}

	std::string result =
		post_content(INOREADER_API_EDIT_TAG_URL, postcontent);

	return result == "OK";
}

std::string
inoreader_api::post_content(const std::string& url, const std::string& postdata)
{
	std::string result;
	curl_slist* custom_headers{};

	CURL* handle = curl_easy_init();
	utils::set_common_curl_options(handle, cfg);
	add_custom_headers(&custom_headers);
	curl_easy_setopt(handle, CURLOPT_HTTPHEADER, custom_headers);
	curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, my_write_data);
	curl_easy_setopt(handle, CURLOPT_WRITEDATA, &result);
	curl_easy_setopt(handle, CURLOPT_POSTFIELDS, postdata.c_str());
	curl_easy_setopt(handle, CURLOPT_URL, url.c_str());
	curl_easy_perform(handle);
	curl_easy_cleanup(handle);
	curl_slist_free_all(custom_headers);

	LOG(level::DEBUG,
	    "inoreader_api::post_content: url = %s postdata = %s result = %s",
	    url,
	    postdata,
	    result);

	return result;
}

} // namespace newsboat

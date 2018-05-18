#include "oldreader_api.h"

#include <cstring>
#include <curl/curl.h>
#include <json.h>
#include <vector>

#include "config.h"
#include "strprintf.h"
#include "utils.h"

#define OLDREADER_LOGIN "https://theoldreader.com/accounts/ClientLogin"
#define OLDREADER_API_PREFIX "http://theoldreader.com/reader/api/0/"
#define OLDREADER_FEED_PREFIX "http://theoldreader.com/reader/atom/"

#define OLDREADER_OUTPUT_SUFFIX "?output=json"

#define OLDREADER_SUBSCRIPTION_LIST \
	OLDREADER_API_PREFIX "subscription/list" OLDREADER_OUTPUT_SUFFIX
#define OLDREADER_API_MARK_ALL_READ_URL OLDREADER_API_PREFIX "mark-all-as-read"
#define OLDREADER_API_EDIT_TAG_URL OLDREADER_API_PREFIX "edit-tag"
#define OLDREADER_API_TOKEN_URL OLDREADER_API_PREFIX "token"

// for reference, see https://github.com/theoldreader/api

namespace newsboat {

oldreader_api::oldreader_api(configcontainer* c)
	: remote_api(c)
{
	// TODO
}

oldreader_api::~oldreader_api()
{
	// TODO
}

bool oldreader_api::authenticate()
{
	auth = retrieve_auth();
	LOG(level::DEBUG, "oldreader_api::authenticate: Auth = %s", auth);
	return auth != "";
}

static size_t
my_write_data(void* buffer, size_t size, size_t nmemb, void* userp)
{
	std::string* pbuf = static_cast<std::string*>(userp);
	pbuf->append(static_cast<const char*>(buffer), size * nmemb);
	return size * nmemb;
}

std::string oldreader_api::retrieve_auth()
{
	CURL* handle = curl_easy_init();
	credentials cred = get_credentials("oldreader", "The Old Reader");
	if (cred.user.empty() || cred.pass.empty()) {
		return "";
	}

	char* username = curl_easy_escape(handle, cred.user.c_str(), 0);
	char* password = curl_easy_escape(handle, cred.pass.c_str(), 0);

	std::string postcontent = strprintf::fmt(
		"service=reader&Email=%s&Passwd=%s&source=%s%2F%s&accountType="
		"HOSTED_OR_GOOGLE&continue=http://www.google.com/",
		username,
		password,
		PROGRAM_NAME,
		PROGRAM_VERSION);

	curl_free(username);
	curl_free(password);

	std::string result;

	utils::set_common_curl_options(handle, cfg);
	curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, my_write_data);
	curl_easy_setopt(handle, CURLOPT_WRITEDATA, &result);
	curl_easy_setopt(handle, CURLOPT_POSTFIELDS, postcontent.c_str());
	curl_easy_setopt(handle, CURLOPT_URL, OLDREADER_LOGIN);
	curl_easy_perform(handle);
	curl_easy_cleanup(handle);

	std::vector<std::string> lines = utils::tokenize(result);
	for (auto line : lines) {
		LOG(level::DEBUG,
			"oldreader_api::retrieve_auth: line = %s",
			line);
		if (line.substr(0, 5) == "Auth=") {
			std::string auth = line.substr(5, line.length() - 5);
			return auth;
		}
	}

	return "";
}

std::vector<tagged_feedurl> oldreader_api::get_subscribed_urls()
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
	curl_easy_setopt(handle, CURLOPT_URL, OLDREADER_SUBSCRIPTION_LIST);
	curl_easy_perform(handle);
	curl_easy_cleanup(handle);
	curl_slist_free_all(custom_headers);

	LOG(level::DEBUG,
		"oldreader_api::get_subscribed_urls: document = %s",
		result);

	json_object* reply = json_tokener_parse(result.c_str());
	if (reply == nullptr) {
		LOG(level::ERROR,
			"oldreader_api::get_subscribed_urls: failed to parse "
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

		// Ignore URLs where ID start with given prefix - those never
		// load, always returning 404 and annoying people
		const char* prefix = "tor/sponsored/";
		if (strncmp(id, prefix, strlen(prefix)) != 0) {
			tags.push_back(std::string("~") + title);

			json_object_object_get_ex(sub, "categories", &node);
			struct array_list* categories =
				json_object_get_array(node);
#if JSON_C_MAJOR_VERSION == 0 && JSON_C_MINOR_VERSION < 13
			for (int i = 0; i < array_list_length(categories);
				i++) {
#else
			for (size_t i = 0; i < array_list_length(categories);
				i++) {
#endif
				json_object* cat =
					json_object_array_get_idx(node, i);
				json_object* label_node{};
				json_object_object_get_ex(
					cat, "label", &label_node);
				const char* label =
					json_object_get_string(label_node);
				tags.push_back(std::string(label));
			}

			auto url = strprintf::fmt("%s%s?n=%u",
				OLDREADER_FEED_PREFIX,
				id,
				cfg->get_configvalue_as_int(
					"oldreader-min-items"));
			urls.push_back(tagged_feedurl(url, tags));
		}
	}

	json_object_put(reply);

	return urls;
}

void oldreader_api::add_custom_headers(curl_slist** custom_headers)
{
	if (auth_header.empty()) {
		auth_header = strprintf::fmt(
			"Authorization: GoogleLogin auth=%s", auth);
	}
	LOG(level::DEBUG,
		"oldreader_api::add_custom_headers header = %s",
		auth_header);
	*custom_headers =
		curl_slist_append(*custom_headers, auth_header.c_str());
}

bool oldreader_api::mark_all_read(const std::string& feedurl)
{
	std::string real_feedurl = feedurl.substr(strlen(OLDREADER_FEED_PREFIX),
		feedurl.length() - strlen(OLDREADER_FEED_PREFIX));
	std::vector<std::string> elems = utils::tokenize(real_feedurl, "?");
	try {
		real_feedurl = utils::unescape_url(elems[0]);
	} catch (const std::runtime_error& e) {
		LOG(level::DEBUG,
			"oldreader_api::mark_all_read: Failed to "
			"unescape_url(%s): "
			"%s",
			elems[0],
			e.what());
		return false;
	}
	std::string token = get_new_token();

	std::string postcontent =
		strprintf::fmt("s=%s&T=%s", real_feedurl, token);

	std::string result =
		post_content(OLDREADER_API_MARK_ALL_READ_URL, postcontent);

	return result == "OK";
}

bool oldreader_api::mark_article_read(const std::string& guid, bool read)
{
	std::string token = get_new_token();
	return mark_article_read_with_token(guid, read, token);
}

bool oldreader_api::mark_article_read_with_token(const std::string& guid,
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
		post_content(OLDREADER_API_EDIT_TAG_URL, postcontent);

	LOG(level::DEBUG,
		"oldreader_api::mark_article_read_with_token: postcontent = %s "
		"result = %s",
		postcontent,
		result);

	return result == "OK";
}

std::string oldreader_api::get_new_token()
{
	CURL* handle = curl_easy_init();
	std::string result;
	curl_slist* custom_headers{};

	utils::set_common_curl_options(handle, cfg);
	add_custom_headers(&custom_headers);
	curl_easy_setopt(handle, CURLOPT_HTTPHEADER, custom_headers);
	curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, my_write_data);
	curl_easy_setopt(handle, CURLOPT_WRITEDATA, &result);
	curl_easy_setopt(handle, CURLOPT_URL, OLDREADER_API_TOKEN_URL);
	curl_easy_perform(handle);
	curl_easy_cleanup(handle);
	curl_slist_free_all(custom_headers);

	LOG(level::DEBUG, "oldreader_api::get_new_token: token = %s", result);

	return result;
}

bool oldreader_api::update_article_flags(const std::string& oldflags,
	const std::string& newflags,
	const std::string& guid)
{
	std::string star_flag = cfg->get_configvalue("oldreader-flag-star");
	std::string share_flag = cfg->get_configvalue("oldreader-flag-share");
	bool success = true;

	if (star_flag.length() > 0) {
		if (strchr(oldflags.c_str(), star_flag[0]) == nullptr &&
			strchr(newflags.c_str(), star_flag[0]) != nullptr) {
			success = star_article(guid, true);
		} else if (strchr(oldflags.c_str(), star_flag[0]) != nullptr &&
			strchr(newflags.c_str(), star_flag[0]) == nullptr) {
			success = star_article(guid, false);
		}
	}

	if (share_flag.length() > 0) {
		if (strchr(oldflags.c_str(), share_flag[0]) == nullptr &&
			strchr(newflags.c_str(), share_flag[0]) != nullptr) {
			success = share_article(guid, true);
		} else if (strchr(oldflags.c_str(), share_flag[0]) != nullptr &&
			strchr(newflags.c_str(), share_flag[0]) == nullptr) {
			success = share_article(guid, false);
		}
	}

	return success;
}

bool oldreader_api::star_article(const std::string& guid, bool star)
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
		post_content(OLDREADER_API_EDIT_TAG_URL, postcontent);

	return result == "OK";
}

bool oldreader_api::share_article(const std::string& guid, bool share)
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
		post_content(OLDREADER_API_EDIT_TAG_URL, postcontent);

	return result == "OK";
}

std::string oldreader_api::post_content(const std::string& url,
	const std::string& postdata)
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
		"oldreader_api::post_content: url = %s postdata = %s result = "
		"%s",
		url,
		postdata,
		result);

	return result;
}

} // namespace newsboat

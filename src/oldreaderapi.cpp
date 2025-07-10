#include "oldreaderapi.h"

#include <cstring>
#include <curl/curl.h>
#include <json.h>
#include <vector>

#include "config.h"
#include "curldatareceiver.h"
#include "curlhandle.h"
#include "logger.h"
#include "strprintf.h"
#include "utils.h"

#define OLDREADER_LOGIN "https://theoldreader.com/accounts/ClientLogin"
#define OLDREADER_API_PREFIX "https://theoldreader.com/reader/api/0/"
#define OLDREADER_FEED_PREFIX "https://theoldreader.com/reader/atom/"

#define OLDREADER_OUTPUT_SUFFIX "?output=json"

#define OLDREADER_SUBSCRIPTION_LIST \
	OLDREADER_API_PREFIX "subscription/list" OLDREADER_OUTPUT_SUFFIX
#define OLDREADER_API_MARK_ALL_READ_URL OLDREADER_API_PREFIX "mark-all-as-read"
#define OLDREADER_API_EDIT_TAG_URL OLDREADER_API_PREFIX "edit-tag"
#define OLDREADER_API_TOKEN_URL OLDREADER_API_PREFIX "token"

// for reference, see https://github.com/theoldreader/api

namespace Newsboat {

OldReaderApi::OldReaderApi(ConfigContainer& c)
	: RemoteApi(c)
{
}

bool OldReaderApi::authenticate()
{
	auth = retrieve_auth();
	LOG(Level::DEBUG, "OldReaderApi::authenticate: Auth = %s", auth);
	return auth != "";
}

std::string OldReaderApi::retrieve_auth()
{
	CurlHandle handle;
	Credentials cred = get_credentials("oldreader", "The Old Reader");
	if (cred.user.empty() || cred.pass.empty()) {
		return "";
	}

	char* username = curl_easy_escape(handle.ptr(), cred.user.c_str(), 0);
	char* password = curl_easy_escape(handle.ptr(), cred.pass.c_str(), 0);

	std::string postcontent = strprintf::fmt(
			"service=reader&Email=%s&Passwd=%s&source=%s%%2F%s&accountType="
			"HOSTED_OR_GOOGLE&continue=http://www.google.com/",
			username,
			password,
			PROGRAM_NAME,
			utils::program_version());

	curl_free(username);
	curl_free(password);

	utils::set_common_curl_options(handle, cfg);
	curl_easy_setopt(handle.ptr(), CURLOPT_POSTFIELDS, postcontent.c_str());
	curl_easy_setopt(handle.ptr(), CURLOPT_URL, OLDREADER_LOGIN);

	auto curlDataReceiver = CurlDataReceiver::register_data_handler(handle);

	curl_easy_perform(handle.ptr());

	const std::string result = curlDataReceiver->get_data();
	std::vector<std::string> lines = utils::tokenize(result);
	for (const auto& line : lines) {
		LOG(Level::DEBUG,
			"OldReaderApi::retrieve_auth: line = %s",
			line);
		if (line.substr(0, 5) == "Auth=") {
			std::string auth = line.substr(5, line.length() - 5);
			return auth;
		}
	}

	return "";
}

std::vector<TaggedFeedUrl> OldReaderApi::get_subscribed_urls()
{
	std::vector<TaggedFeedUrl> urls;
	curl_slist* custom_headers{};

	CurlHandle handle;
	add_custom_headers(&custom_headers);
	curl_easy_setopt(handle.ptr(), CURLOPT_HTTPHEADER, custom_headers);

	utils::set_common_curl_options(handle, cfg);
	curl_easy_setopt(handle.ptr(), CURLOPT_URL, OLDREADER_SUBSCRIPTION_LIST);

	auto curlDataReceiver = CurlDataReceiver::register_data_handler(handle);

	curl_easy_perform(handle.ptr());
	curl_slist_free_all(custom_headers);

	const std::string result = curlDataReceiver->get_data();
	LOG(Level::DEBUG,
		"OldReaderApi::get_subscribed_urls: document = %s",
		result);

	json_object* reply = json_tokener_parse(result.c_str());
	if (reply == nullptr) {
		LOG(Level::ERROR,
			"OldReaderApi::get_subscribed_urls: failed to parse "
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
		if (id == nullptr) {
			LOG(Level::WARN, "Skipping a subscription without an id");
			continue;
		}

		json_object_object_get_ex(sub, "title", &node);
		const char* title_ptr = json_object_get_string(node);
		std::string title;
		if (title_ptr != nullptr) {
			title = title_ptr;
		} else {
			LOG(Level::WARN, "Subscription has no title, so let's call it \"%i\"", i);
			title = std::to_string(i);
		}

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
				const char* label = json_object_get_string(label_node);
				if (label == nullptr) {
					LOG(Level::WARN, "Skipping subscription's label whose name is a null value");
					continue;
				}
				tags.push_back(std::string(label));
			}

			auto url = strprintf::fmt("%s%s?n=%u",
					OLDREADER_FEED_PREFIX,
					id,
					cfg.get_configvalue_as_int("oldreader-min-items"));
			urls.push_back(TaggedFeedUrl(url, tags));
		}
	}

	json_object_put(reply);

	return urls;
}

void OldReaderApi::add_custom_headers(curl_slist** custom_headers)
{
	if (auth_header.empty()) {
		auth_header = strprintf::fmt(
				"Authorization: GoogleLogin auth=%s", auth);
	}
	LOG(Level::DEBUG,
		"OldReaderApi::add_custom_headers header = %s",
		auth_header);
	*custom_headers =
		curl_slist_append(*custom_headers, auth_header.c_str());
}

bool OldReaderApi::mark_all_read(const std::string& feedurl)
{
	std::string real_feedurl = feedurl.substr(strlen(OLDREADER_FEED_PREFIX),
			feedurl.length() - strlen(OLDREADER_FEED_PREFIX));
	std::vector<std::string> elems = utils::tokenize(real_feedurl, "?");
	try {
		real_feedurl = utils::unescape_url(elems[0]);
	} catch (const std::runtime_error& e) {
		LOG(Level::DEBUG,
			"OldReaderApi::mark_all_read: Failed to "
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

bool OldReaderApi::mark_article_read(const std::string& guid, bool read)
{
	std::string token = get_new_token();
	return mark_article_read_with_token(guid, read, token);
}

bool OldReaderApi::mark_article_read_with_token(const std::string& guid,
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

	LOG(Level::DEBUG,
		"OldReaderApi::mark_article_read_with_token: postcontent = %s "
		"result = %s",
		postcontent,
		result);

	return result == "OK";
}

std::string OldReaderApi::get_new_token()
{
	CurlHandle handle;
	curl_slist* custom_headers{};

	utils::set_common_curl_options(handle, cfg);
	add_custom_headers(&custom_headers);
	curl_easy_setopt(handle.ptr(), CURLOPT_HTTPHEADER, custom_headers);
	curl_easy_setopt(handle.ptr(), CURLOPT_URL, OLDREADER_API_TOKEN_URL);

	auto curlDataReceiver = CurlDataReceiver::register_data_handler(handle);

	curl_easy_perform(handle.ptr());
	curl_slist_free_all(custom_headers);

	const std::string result = curlDataReceiver->get_data();
	LOG(Level::DEBUG, "OldReaderApi::get_new_token: token = %s", result);

	return result;
}

bool OldReaderApi::update_article_flags(const std::string& oldflags,
	const std::string& newflags,
	const std::string& guid)
{
	std::string star_flag = cfg.get_configvalue("oldreader-flag-star");
	std::string share_flag = cfg.get_configvalue("oldreader-flag-share");
	bool success = true;

	if (star_flag.length() > 0) {
		update_flag(oldflags, newflags, star_flag[0], [&](bool added) {
			success = star_article(guid, added);
		});
	}

	if (share_flag.length() > 0) {
		update_flag(oldflags, newflags, share_flag[0], [&](bool added) {
			success = share_article(guid, added);
		});
	}

	return success;
}

bool OldReaderApi::star_article(const std::string& guid, bool star)
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

bool OldReaderApi::share_article(const std::string& guid, bool share)
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

std::string OldReaderApi::post_content(const std::string& url,
	const std::string& postdata)
{
	curl_slist* custom_headers{};

	CurlHandle handle;
	utils::set_common_curl_options(handle, cfg);
	add_custom_headers(&custom_headers);
	curl_easy_setopt(handle.ptr(), CURLOPT_HTTPHEADER, custom_headers);
	curl_easy_setopt(handle.ptr(), CURLOPT_POSTFIELDS, postdata.c_str());
	curl_easy_setopt(handle.ptr(), CURLOPT_URL, url.c_str());

	auto curlDataReceiver = CurlDataReceiver::register_data_handler(handle);

	curl_easy_perform(handle.ptr());
	curl_slist_free_all(custom_headers);

	const std::string result = curlDataReceiver->get_data();
	LOG(Level::DEBUG,
		"OldReaderApi::post_content: url = %s postdata = %s result = "
		"%s",
		url,
		postdata,
		result);

	return result;
}

} // namespace Newsboat

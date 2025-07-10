#include "feedhqapi.h"

#include <curl/curl.h>
#include <json.h>
#include <vector>

#include "config.h"
#include "curldatareceiver.h"
#include "curlhandle.h"
#include "logger.h"
#include "strprintf.h"
#include "utils.h"

#define FEEDHQ_LOGIN "/accounts/ClientLogin"
#define FEEDHQ_API_PREFIX "/reader/api/0/"
#define FEEDHQ_FEED_PREFIX "/reader/atom/"

#define FEEDHQ_OUTPUT_SUFFIX "?output=json"

#define FEEDHQ_SUBSCRIPTION_LIST \
	FEEDHQ_API_PREFIX "subscription/list" FEEDHQ_OUTPUT_SUFFIX
#define FEEDHQ_API_MARK_ALL_READ_URL FEEDHQ_API_PREFIX "mark-all-as-read"
#define FEEDHQ_API_EDIT_TAG_URL FEEDHQ_API_PREFIX "edit-tag"
#define FEEDHQ_API_TOKEN_URL FEEDHQ_API_PREFIX "token"

namespace Newsboat {

FeedHqApi::FeedHqApi(ConfigContainer& c)
	: RemoteApi(c)
{
}

bool FeedHqApi::authenticate()
{
	auth = retrieve_auth();
	LOG(Level::DEBUG, "FeedHqApi::authenticate: Auth = %s", auth);
	return auth != "";
}

std::string FeedHqApi::retrieve_auth()
{
	CurlHandle handle;

	Credentials cred = get_credentials("feedhq", "FeedHQ");
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
	curl_easy_setopt(handle.ptr(),
		CURLOPT_URL,
		(cfg.get_configvalue("feedhq-url") + FEEDHQ_LOGIN).c_str());

	auto curlDataReceiver = CurlDataReceiver::register_data_handler(handle);

	curl_easy_perform(handle.ptr());

	const auto result = curlDataReceiver->get_data();
	for (const auto& line : utils::tokenize(result)) {
		LOG(Level::DEBUG, "FeedHqApi::retrieve_auth: line = %s", line);
		if (line.substr(0, 5) == "Auth=") {
			std::string auth = line.substr(5, line.length() - 5);
			return auth;
		}
	}

	return "";
}

std::vector<TaggedFeedUrl> FeedHqApi::get_subscribed_urls()
{
	std::vector<TaggedFeedUrl> urls;

	CurlHandle handle;
	curl_slist* custom_headers{};
	add_custom_headers(&custom_headers);
	curl_easy_setopt(handle.ptr(), CURLOPT_HTTPHEADER, custom_headers);

	utils::set_common_curl_options(handle, cfg);
	curl_easy_setopt(handle.ptr(),
		CURLOPT_URL,
		(cfg.get_configvalue("feedhq-url") + FEEDHQ_SUBSCRIPTION_LIST)
		.c_str());
	auto curlDataReceiver = CurlDataReceiver::register_data_handler(handle);

	curl_easy_perform(handle.ptr());
	curl_slist_free_all(custom_headers);

	const auto result = curlDataReceiver->get_data();
	LOG(Level::DEBUG,
		"FeedHqApi::get_subscribed_urls: document = %s",
		result);

	json_object* reply = json_tokener_parse(result.c_str());
	if (reply == nullptr) {
		LOG(Level::ERROR,
			"FeedHqApi::get_subscribed_urls: failed to parse "
			"response "
			"as JSON.");
		return urls;
	}

	json_object* subscription_obj{};
	json_object_object_get_ex(reply, "subscriptions", &subscription_obj);
	array_list* subscriptions = json_object_get_array(subscription_obj);

	int len = array_list_length(subscriptions);

	for (int i = 0; i < len; i++) {
		std::vector<std::string> tags;
		json_object* sub = json_object_array_get_idx(subscription_obj, i);

		json_object* id_str{};
		json_object_object_get_ex(sub, "id", &id_str);
		const char* id = json_object_get_string(id_str);
		if (id == nullptr) {
			LOG(Level::WARN, "Skipping a subscription without an id");
			continue;
		}

		json_object* title_str{};
		json_object_object_get_ex(sub, "title", &title_str);
		const char* title = json_object_get_string(title_str);
		if (title != nullptr) {
			tags.push_back(std::string("~") + title);
		} else {
			LOG(Level::WARN, "Subscription has no title, so let's call it \"%i\"", i);
			tags.push_back(std::string("~") + std::to_string(i));
		}

		char* escaped_id = curl_easy_escape(handle.ptr(), id, 0);

		auto url = strprintf::fmt("%s%s%s?n=%u",
				cfg.get_configvalue("feedhq-url"),
				FEEDHQ_FEED_PREFIX,
				escaped_id,
				cfg.get_configvalue_as_int("feedhq-min-items"));
		urls.push_back(TaggedFeedUrl(url, tags));

		curl_free(escaped_id);
	}

	json_object_put(reply);

	return urls;
}

void FeedHqApi::add_custom_headers(curl_slist** custom_headers)
{
	if (auth_header.empty()) {
		auth_header = strprintf::fmt(
				"Authorization: GoogleLogin auth=%s", auth);
	}
	LOG(Level::DEBUG,
		"FeedHqApi::add_custom_headers header = %s",
		auth_header);
	*custom_headers =
		curl_slist_append(*custom_headers, auth_header.c_str());
}

bool FeedHqApi::mark_all_read(const std::string& feedurl)
{
	std::string prefix =
		cfg.get_configvalue("feedhq-url") + FEEDHQ_FEED_PREFIX;
	std::string real_feedurl = feedurl.substr(
			prefix.length(), feedurl.length() - prefix.length());
	std::vector<std::string> elems = utils::tokenize(real_feedurl, "?");
	try {
		real_feedurl = utils::unescape_url(elems[0]);
	} catch (const std::runtime_error& e) {
		LOG(Level::DEBUG,
			"FeedHqApi::mark_all_read: Failed to "
			"unescape_url(%s): %s",
			elems[0],
			e.what());
		return false;
	}
	std::string token = get_new_token();

	std::string postcontent =
		strprintf::fmt("s=%s&T=%s", real_feedurl, token);

	std::string result = post_content(cfg.get_configvalue("feedhq-url") +
			FEEDHQ_API_MARK_ALL_READ_URL,
			postcontent);

	return result == "OK";
}

bool FeedHqApi::mark_article_read(const std::string& guid, bool read)
{
	std::string token = get_new_token();
	return mark_article_read_with_token(guid, read, token);
}

bool FeedHqApi::mark_article_read_with_token(const std::string& guid,
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

	std::string result = post_content(
			cfg.get_configvalue("feedhq-url") + FEEDHQ_API_EDIT_TAG_URL,
			postcontent);

	LOG(Level::DEBUG,
		"FeedHqApi::mark_article_read_with_token: postcontent = %s "
		"result "
		"= %s",
		postcontent,
		result);

	return result == "OK";
}

std::string FeedHqApi::get_new_token()
{
	CurlHandle handle;
	curl_slist* custom_headers{};

	utils::set_common_curl_options(handle, cfg);
	add_custom_headers(&custom_headers);
	curl_easy_setopt(handle.ptr(), CURLOPT_HTTPHEADER, custom_headers);
	curl_easy_setopt(handle.ptr(),
		CURLOPT_URL,
		(cfg.get_configvalue("feedhq-url") + FEEDHQ_API_TOKEN_URL)
		.c_str());

	auto curlDataReceiver = CurlDataReceiver::register_data_handler(handle);

	curl_easy_perform(handle.ptr());
	curl_slist_free_all(custom_headers);

	const auto result = curlDataReceiver->get_data();
	LOG(Level::DEBUG, "FeedHqApi::get_new_token: token = %s", result);

	return result;
}

bool FeedHqApi::update_article_flags(const std::string& oldflags,
	const std::string& newflags,
	const std::string& guid)
{
	std::string star_flag = cfg.get_configvalue("feedhq-flag-star");
	std::string share_flag = cfg.get_configvalue("feedhq-flag-share");
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

bool FeedHqApi::star_article(const std::string& guid, bool star)
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

	std::string result = post_content(
			cfg.get_configvalue("feedhq-url") + FEEDHQ_API_EDIT_TAG_URL,
			postcontent);

	return result == "OK";
}

bool FeedHqApi::share_article(const std::string& guid, bool share)
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

	std::string result = post_content(
			cfg.get_configvalue("feedhq-url") + FEEDHQ_API_EDIT_TAG_URL,
			postcontent);

	return result == "OK";
}

std::string FeedHqApi::post_content(const std::string& url,
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

	const auto result = curlDataReceiver->get_data();
	LOG(Level::DEBUG,
		"FeedHqApi::post_content: url = %s postdata = %s result = %s",
		url,
		postdata,
		result);

	return result;
}

} // namespace Newsboat

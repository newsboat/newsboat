#include "inoreaderapi.h"

#include <cstring>
#include <curl/curl.h>
#include <vector>
#include <thread>

#include "3rd-party/json.hpp"
#include "curldatareceiver.h"
#include "curlhandle.h"
#include "logger.h"
#include "strprintf.h"
#include "utils.h"

#define INOREADER_LOGIN "https://inoreader.com/accounts/ClientLogin"
#define INOREADER_API_PREFIX "https://inoreader.com/reader/api/0/"
#define INOREADER_FEED_PREFIX "https://inoreader.com/reader/atom/"

#define INOREADER_SUBSCRIPTION_LIST INOREADER_API_PREFIX "subscription/list"
#define INOREADER_API_MARK_ALL_READ_URL INOREADER_API_PREFIX "mark-all-as-read"
#define INOREADER_API_EDIT_TAG_URL INOREADER_API_PREFIX "edit-tag"

using json = nlohmann::json;

// for reference, see https://inoreader.com/developers

namespace newsboat {

InoreaderApi::InoreaderApi(ConfigContainer& c)
	: RemoteApi(c)
{
}

bool InoreaderApi::authenticate()
{
	auth = retrieve_auth();
	LOG(Level::DEBUG, "InoreaderApi::authenticate: Auth = %s", auth);
	return auth != "";
}

std::string InoreaderApi::retrieve_auth()
{
	CurlHandle handle;
	Credentials cred = get_credentials("inoreader", "Inoreader");
	if (cred.user.empty() || cred.pass.empty()) {
		return "";
	}

	char* username = curl_easy_escape(handle.ptr(), cred.user.c_str(), 0);
	char* password = curl_easy_escape(handle.ptr(), cred.pass.c_str(), 0);

	std::string postcontent =
		strprintf::fmt("Email=%s&Passwd=%s", username, password);

	curl_free(username);
	curl_free(password);

	curl_slist* list = NULL;
	list = add_app_headers(list);

	utils::set_common_curl_options(handle, cfg);
	curl_easy_setopt(handle.ptr(), CURLOPT_POSTFIELDS, postcontent.c_str());
	curl_easy_setopt(handle.ptr(), CURLOPT_URL, INOREADER_LOGIN);

	auto curlDataReceiver = CurlDataReceiver::register_data_handler(handle);

	curl_easy_perform(handle.ptr());
	curl_slist_free_all(list);

	const std::string result = curlDataReceiver->get_data();
	std::vector<std::string> lines = utils::tokenize(result);
	for (const auto& line : lines) {
		LOG(Level::DEBUG,
			"InoreaderApi::retrieve_auth: line = %s",
			line);
		const std::string auth_string = "Auth=";
		if (line.compare(0, auth_string.length(), auth_string) == 0) {
			std::string auth = line.substr(auth_string.length());
			return auth;
		}
	}

	return "";
}

std::vector<TaggedFeedUrl> InoreaderApi::get_subscribed_urls()
{
	std::vector<TaggedFeedUrl> urls;
	curl_slist* custom_headers{};

	CurlHandle handle;
	add_custom_headers(&custom_headers);
	curl_easy_setopt(handle.ptr(), CURLOPT_HTTPHEADER, custom_headers);

	utils::set_common_curl_options(handle, cfg);
	curl_easy_setopt(handle.ptr(), CURLOPT_URL, INOREADER_SUBSCRIPTION_LIST);

	auto curlDataReceiver = CurlDataReceiver::register_data_handler(handle);

	curl_easy_perform(handle.ptr());
	curl_slist_free_all(custom_headers);

	const std::string result = curlDataReceiver->get_data();
	LOG(Level::DEBUG,
		"InoreaderApi::get_subscribed_urls: document = %s",
		result);

	json reply;
	try {
		reply = json::parse(result);
	} catch (json::parse_error& e) {
		LOG(Level::ERROR,
			"InoreaderApi::get_subscribed_urls: failed to parse response as JSON: %s", e.what());
		return urls;
	}

	try {
		const json& subscriptions = reply.at("subscriptions");

		for (std::size_t i = 0; i < subscriptions.size(); i++) {
			std::vector<std::string> tags;
			const json& sub = subscriptions.at(i);

			if (!sub.contains("id") || sub.at("id").is_null()) {
				LOG(Level::WARN, "Skipping a subscription without an id");
				continue;
			}

			const std::string id = sub.at("id");

			if (sub.contains("title") && !sub.at("title").is_null()) {
				const std::string title = sub.at("title");
				tags.push_back(std::string("~") + title);
			} else {
				LOG(Level::WARN, "Subscription has no title, so let's call it \"%zu\"", i);
				tags.push_back(std::string("~") + std::to_string(i));
			}

			const json& categories = sub.at("categories");

			for (const auto& cat : categories) {
				if (!cat.contains("label") || cat.at("label").is_null()) {
					LOG(Level::WARN, "Skipping subscription's label whose name is a null value");
					continue;
				}

				const std::string label = cat.at("label");
				tags.push_back(label);
			}

			char* id_uenc = curl_easy_escape(handle.ptr(), id.c_str(), 0);

			auto url = strprintf::fmt("%s%s?n=%u",
					INOREADER_FEED_PREFIX,
					id_uenc,
					cfg.get_configvalue_as_int("inoreader-min-items"));
			urls.push_back(TaggedFeedUrl(url, tags));

			curl_free(id_uenc);
		}
	} catch (json::exception& e) {
		LOG(Level::ERROR, "InoreaderApi::get_subscribed_urls: failed to parse subscriptions: %s",
			e.what());
		return urls;
	}

	return urls;
}

void InoreaderApi::add_custom_headers(curl_slist** custom_headers)
{
	if (auth_header.empty()) {
		auth_header = strprintf::fmt(
				"Authorization: GoogleLogin auth=%s", auth);
	}
	LOG(Level::DEBUG,
		"InoreaderApi::add_custom_headers header = %s",
		auth_header);
	*custom_headers =
		curl_slist_append(*custom_headers, auth_header.c_str());
	*custom_headers = add_app_headers(*custom_headers);
}

bool InoreaderApi::mark_all_read(const std::string& feedurl)
{
	std::string real_feedurl =
		feedurl.substr(strlen(INOREADER_FEED_PREFIX));
	std::vector<std::string> elems = utils::tokenize(real_feedurl, "?");
	real_feedurl = utils::unescape_url(elems[0]);

	std::string postcontent = strprintf::fmt("s=%s", real_feedurl);

	std::string result =
		post_content(INOREADER_API_MARK_ALL_READ_URL, postcontent);

	return result == "OK";
}

bool InoreaderApi::mark_article_read(const std::string& guid, bool read)
{
	// Do this in a thread, as we don't care about the result enough to wait
	// for it.  borrowed from ttrssapi.cpp
	std::thread t{[=]()
	{
		LOG(Level::DEBUG,
			"InoreaderApi::mark_article_read: inside thread, marking "
			"thread as read...");

		std::string postcontent;

		if (read) {
			postcontent = strprintf::fmt(
					"i=%s&a=user/-/state/com.google/read",
					guid);
		} else {
			postcontent = strprintf::fmt(
					"i=%s&r=user/-/state/com.google/read",
					guid);
		}

		std::string result =
			post_content(INOREADER_API_EDIT_TAG_URL, postcontent);

		LOG(Level::DEBUG,
			"InoreaderApi::mark_article_read: postcontent = %s result = %s",
			postcontent,
			result);

		return result == "OK";
	}};
	t.detach();
	return true;
}

bool InoreaderApi::update_article_flags(const std::string& inoflags,
	const std::string& newflags,
	const std::string& guid)
{
	std::string star_flag = cfg.get_configvalue("inoreader-flag-star");
	std::string share_flag = cfg.get_configvalue("inoreader-flag-share");
	bool success = true;

	if (star_flag.length() > 0) {
		update_flag(inoflags, newflags, star_flag[0], [&](bool added) {
			success = star_article(guid, added);
		});
	}

	if (share_flag.length() > 0) {
		update_flag(inoflags, newflags, share_flag[0], [&](bool added) {
			success = share_article(guid, added);
		});
	}

	return success;
}

bool InoreaderApi::star_article(const std::string& guid, bool star)
{
	std::string postcontent;

	if (star) {
		postcontent = strprintf::fmt(
				"i=%s&a=user/-/state/com.google/starred&ac=edit",
				guid);
	} else {
		postcontent = strprintf::fmt(
				"i=%s&r=user/-/state/com.google/starred&ac=edit",
				guid);
	}

	std::string result =
		post_content(INOREADER_API_EDIT_TAG_URL, postcontent);

	return result == "OK";
}

bool InoreaderApi::share_article(const std::string& guid, bool share)
{
	std::string postcontent;

	if (share) {
		postcontent = strprintf::fmt(
				"i=%s&a=user/-/state/com.google/broadcast&ac=edit",
				guid);
	} else {
		postcontent = strprintf::fmt(
				"i=%s&r=user/-/state/com.google/broadcast&ac=edit",
				guid);
	}

	std::string result =
		post_content(INOREADER_API_EDIT_TAG_URL, postcontent);

	return result == "OK";
}

std::string InoreaderApi::post_content(const std::string& url,
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
		"InoreaderApi::post_content: url = %s postdata = %s result = "
		"%s",
		url,
		postdata,
		result);

	return result;
}

curl_slist* InoreaderApi::add_app_headers(curl_slist* headers)
{
	const auto app_id = strprintf::fmt(
			"AppId: %s",
			cfg.get_configvalue("inoreader-app-id"));
	headers = curl_slist_append(headers, app_id.c_str());

	const auto app_key = strprintf::fmt(
			"AppKey: %s",
			cfg.get_configvalue("inoreader-app-key"));
	headers = curl_slist_append(headers, app_key.c_str());

	return headers;
}

} // namespace newsboat

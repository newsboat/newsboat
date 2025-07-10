#include "freshrssapi.h"

#include <curl/curl.h>
#include <json.h>
#include <time.h>
#include <vector>
#include <thread>

#include "config.h"
#include "curldatareceiver.h"
#include "curlhandle.h"
#include "logger.h"
#include "strprintf.h"
#include "utils.h"
#include "rss/feed.h"
#include "3rd-party/json.hpp"

#define FRESHRSS_LOGIN "/accounts/ClientLogin"
#define FRESHRSS_API_PREFIX "/reader/api/0/"
#define FRESHRSS_FEED_PREFIX "/reader/api/0/stream/contents/"

#define FRESHRSS_OUTPUT_SUFFIX "?output=json"

#define FRESHRSS_SUBSCRIPTION_LIST \
	FRESHRSS_API_PREFIX "subscription/list" FRESHRSS_OUTPUT_SUFFIX
#define FRESHRSS_API_MARK_ALL_READ_URL FRESHRSS_API_PREFIX "mark-all-as-read"
#define FRESHRSS_API_EDIT_TAG_URL FRESHRSS_API_PREFIX "edit-tag"
#define FRESHRSS_API_TOKEN_URL FRESHRSS_API_PREFIX "token"

namespace Newsboat {

FreshRssApi::FreshRssApi(ConfigContainer& c)
	: RemoteApi(c)
{
	token_expired = true;
}

bool FreshRssApi::authenticate()
{
	auth = retrieve_auth();
	LOG(Level::DEBUG, "FreshRssApi::authenticate: Auth = %s", auth);
	return auth != "";
}

std::string FreshRssApi::retrieve_auth()
{
	CurlHandle handle;
	Credentials cred = get_credentials("freshrss", "FreshRSS");
	if (cred.user.empty() || cred.pass.empty()) {
		return "";
	}

	char* username = curl_easy_escape(handle.ptr(), cred.user.c_str(), 0);
	char* password = curl_easy_escape(handle.ptr(), cred.pass.c_str(), 0);

	std::string postcontent = strprintf::fmt(
			"Email=%s&Passwd=%s&source=%s%%2F%s",
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
		(cfg.get_configvalue("freshrss-url") + FRESHRSS_LOGIN).c_str());

	auto curlDataReceiver = CurlDataReceiver::register_data_handler(handle);

	curl_easy_perform(handle.ptr());

	const std::string result = curlDataReceiver->get_data();
	for (const auto& line : utils::tokenize(result)) {
		LOG(Level::DEBUG, "FreshRssApi::retrieve_auth: line = %s", line);
		if (line.substr(0, 5) == "Auth=") {
			std::string auth = line.substr(5, line.length() - 5);
			return auth;
		}
	}

	return "";
}

std::vector<TaggedFeedUrl> FreshRssApi::get_subscribed_urls()
{
	std::vector<TaggedFeedUrl> urls;

	CurlHandle handle;
	curl_slist* custom_headers{};
	add_custom_headers(&custom_headers);
	curl_easy_setopt(handle.ptr(), CURLOPT_HTTPHEADER, custom_headers);

	utils::set_common_curl_options(handle, cfg);
	curl_easy_setopt(handle.ptr(),
		CURLOPT_URL,
		(cfg.get_configvalue("freshrss-url") + FRESHRSS_SUBSCRIPTION_LIST)
		.c_str());

	auto curlDataReceiver = CurlDataReceiver::register_data_handler(handle);

	curl_easy_perform(handle.ptr());
	curl_slist_free_all(custom_headers);

	const std::string result = curlDataReceiver->get_data();
	LOG(Level::DEBUG,
		"FreshRssApi::get_subscribed_urls: document = %s",
		result);

	json_object* reply = json_tokener_parse(result.c_str());
	if (reply == nullptr) {
		LOG(Level::ERROR,
			"FreshRssApi::get_subscribed_urls: failed to parse "
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
		json_object* sub =
			json_object_array_get_idx(subscription_obj, i);

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

		json_object* cats_obj{};
		json_object_object_get_ex(sub, "categories", &cats_obj);
		array_list* cats = json_object_get_array(cats_obj);

		int ncats = array_list_length(cats);
		for (int x = 0; x < ncats; x++) {
			json_object* cat = json_object_array_get_idx(cats_obj, x);
			json_object* cat_name{};
			json_object_object_get_ex(cat, "label", &cat_name);
			const char* category = json_object_get_string(cat_name);
			if (category == nullptr) {
				LOG(Level::WARN, "Skipping subscription's category whose name is a null value");
				continue;
			}
			tags.push_back(category);
		}

		char* escaped_id = curl_easy_escape(handle.ptr(), id, 0);
		auto url = strprintf::fmt("%s%s%s",
				cfg.get_configvalue("freshrss-url"),
				FRESHRSS_FEED_PREFIX,
				escaped_id);
		urls.push_back(TaggedFeedUrl(url, tags));
		curl_free(escaped_id);
	}

	json_object_put(reply);

	return urls;
}

void FreshRssApi::add_custom_headers(curl_slist** custom_headers)
{
	if (auth_header.empty()) {
		auth_header = strprintf::fmt(
				"Authorization: GoogleLogin auth=%s", auth);
	}
	LOG(Level::DEBUG,
		"FreshRssApi::add_custom_headers header = %s",
		auth_header);
	*custom_headers =
		curl_slist_append(*custom_headers, auth_header.c_str());
}

bool FreshRssApi::mark_all_read(const std::string& feedurl)
{
	std::string prefix =
		cfg.get_configvalue("freshrss-url") + FRESHRSS_FEED_PREFIX;
	std::string real_feedurl = feedurl.substr(
			prefix.length(), feedurl.length() - prefix.length());
	std::vector<std::string> elems = utils::tokenize(real_feedurl, "?");
	try {
		real_feedurl = utils::unescape_url(elems[0]);
	} catch (const std::runtime_error& e) {
		LOG(Level::DEBUG,
			"FreshRssApi::mark_all_read: Failed to "
			"unescape_url(%s): %s",
			elems[0],
			e.what());
		return false;
	}

	refresh_token();

	std::string postcontent =
		strprintf::fmt("s=%s&T=%s", real_feedurl, token);

	std::string result = post_content(cfg.get_configvalue("freshrss-url") +
			FRESHRSS_API_MARK_ALL_READ_URL,
			postcontent);

	return result == "OK";
}

bool FreshRssApi::mark_article_read(const std::string& guid, bool read)
{
	refresh_token();

	std::thread t{[=]()
	{
		LOG(Level::DEBUG,
			"FreshRssApi::mark_article_read: inside thread, marking "
			"article as read...");
		mark_article_read_with_token(guid, read, token);
	}};

	t.detach();
	return true;
}

bool FreshRssApi::mark_article_read_with_token(const std::string& guid,
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
			cfg.get_configvalue("freshrss-url") + FRESHRSS_API_EDIT_TAG_URL,
			postcontent);

	LOG(Level::DEBUG,
		"FreshRssApi::mark_article_read_with_token: postcontent = %s "
		"result "
		"= %s",
		postcontent,
		result);

	return result == "OK";
}

std::string FreshRssApi::get_new_token()
{
	CurlHandle handle;
	curl_slist* custom_headers{};

	utils::set_common_curl_options(handle, cfg);
	add_custom_headers(&custom_headers);
	curl_easy_setopt(handle.ptr(), CURLOPT_HTTPHEADER, custom_headers);
	curl_easy_setopt(handle.ptr(),
		CURLOPT_URL,
		(cfg.get_configvalue("freshrss-url") + FRESHRSS_API_TOKEN_URL)
		.c_str());

	auto curlDataReceiver = CurlDataReceiver::register_data_handler(handle);

	curl_easy_perform(handle.ptr());
	curl_slist_free_all(custom_headers);

	const std::string result = curlDataReceiver->get_data();
	LOG(Level::DEBUG, "FreshRssApi::get_new_token: token = %s", result);

	return result;
}

bool FreshRssApi::refresh_token()
{
	// Check and, if needed, refresh token
	// Note that at present token never expires
	// https://github.com/FreshRSS/FreshRSS/blob/634005de9a4b5e415ebf7c1106c769a0fbed5cfd/p/api/greader.php#L206
	if (token_expired) {
		token = get_new_token();
		token_expired = false;
		return true;
	}
	return false;
}

bool FreshRssApi::update_article_flags(const std::string& oldflags,
	const std::string& newflags,
	const std::string& guid)
{
	std::string star_flag = cfg.get_configvalue("freshrss-flag-star");
	std::string share_flag = cfg.get_configvalue("freshrss-flag-share");
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

bool FreshRssApi::star_article(const std::string& guid, bool star)
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
			cfg.get_configvalue("freshrss-url") + FRESHRSS_API_EDIT_TAG_URL,
			postcontent);

	return result == "OK";
}

bool FreshRssApi::share_article(const std::string& guid, bool share)
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
			cfg.get_configvalue("freshrss-url") + FRESHRSS_API_EDIT_TAG_URL,
			postcontent);

	return result == "OK";
}

std::string FreshRssApi::post_content(const std::string& url,
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
		"FreshRssApi::post_content: url = %s postdata = %s result = %s",
		url,
		postdata,
		result);

	return result;
}

rsspp::Feed FreshRssApi::fetch_feed(const std::string& id, CurlHandle& cached_handle)
{
	rsspp::Feed feed;
	feed.rss_version = rsspp::Feed::FRESHRSS_JSON;

	const std::string query = strprintf::fmt("%s?n=%u",
			id,
			cfg.get_configvalue_as_int("freshrss-min-items"));

	curl_slist* custom_headers{};
	add_custom_headers(&custom_headers);
	curl_easy_setopt(cached_handle.ptr(), CURLOPT_HTTPHEADER, custom_headers);

	utils::set_common_curl_options(cached_handle, cfg);
	curl_easy_setopt(cached_handle.ptr(),
		CURLOPT_URL,
		query.c_str());

	auto curlDataReceiver = CurlDataReceiver::register_data_handler(cached_handle);

	curl_easy_perform(cached_handle.ptr());

	curl_slist_free_all(custom_headers);

	const std::string result = curlDataReceiver->get_data();
	if (result.empty()) {
		LOG(Level::ERROR,
			"FreshRssApi::fetch_feed: Empty response: %s",
			result);
		return feed;
	}
	nlohmann::json content;
	try {
		content = nlohmann::json::parse(result);
	} catch (nlohmann::json::parse_error& e) {
		LOG(Level::ERROR,
			"FreshRssApi::fetch_feed: reply failed to parse: %s",
			result);
		return feed;
	}


	const nlohmann::json entries = content["items"];
	if (!entries.is_array()) {
		LOG(Level::ERROR,
			"FreshRssApi::fetch_feed: items is not an array");
		return feed;
	}

	LOG(Level::DEBUG,
		"FreshRssApi::fetch_feed: %" PRIu64 " items",
		static_cast<uint64_t>(entries.size()));
	try {
		for (const auto& entry : entries) {
			rsspp::Item item;

			// Title
			if (entry.contains("title") && !entry["title"].is_null()) {
				item.title = entry["title"];
			}

			// Link
			if (entry.contains("canonical") && !entry["canonical"].is_null()) {
				for (const auto& a : entry["canonical"]) {
					if (a.contains("href") && !a["href"].is_null()) {
						item.link = a["href"];
						break;
					}
				}
			}

			// Author
			if (entry.contains("author") && !entry["author"].is_null()) {
				item.author = entry["author"];
			}

			// Content
			if (entry.contains("summary") && !entry["summary"].is_null()) {
				for (const auto& a : entry["summary"].items()) {
					if (!a.value().is_null()) {
						item.content_encoded = a.value();
						break;
					}
				}
			}

			// Guid
			if (entry.contains("id") && !entry["id"].is_null()) {
				item.guid = entry["id"];
			}

			// Publish date
			if (entry.contains("published") && !entry["published"].is_null()) {
				int pub_time = entry["published"];
				time_t updated = static_cast<time_t>(pub_time);

				item.pubDate = utils::mt_strf_localtime(
						"%a, %d %b %Y %H:%M:%S %z",
						updated);
				item.pubDate_ts = pub_time;
			}

			// Podcast enclosure
			if (entry.contains("enclosure") && !entry["enclosure"].is_null()) {
				for (const auto& a : entry["enclosure"]) {
					if (a.contains("href") && a.contains("type")
						&& !a["href"].is_null() && !a["type"].is_null()) {
						item.enclosures.push_back(
						rsspp::Enclosure {
							a["href"],
							a["type"],
							"",
							"",
						}
						);
						break;
					}
				}
			}

			// Read/unread status
			bool unread = true;
			if (entry.contains("categories")
				&& !entry["categories"].is_null()) {
				for (const auto& a: entry["categories"]) {
					if (a == "user/-/state/com.google/read") {
						unread = false;
					}
				}
			}
			if (unread) {
				item.labels.push_back("unread");
			} else {
				item.labels.push_back("read");
			}

			feed.items.push_back(item);
		}
	} catch (nlohmann::json::exception& e) {
		LOG(Level::ERROR, "Exception occurred while parsing feed: ", e.what());
	}

	return feed;
}

} // namespace Newsboat

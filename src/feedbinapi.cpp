#include "feedbinapi.h"

#include <cinttypes>
#include <cstdint>
#include <curl/curl.h>
#include <string>

#include "3rd-party/json.hpp"
#include "curlhandle.h"
#include "logger.h"
#include "rss/feed.h"
#include "strprintf.h"
#include "utils.h"

#define FEEDBIN_AUTHENTICATION_PATH "/v2/authentication.json"
#define FEEDBIN_STARRED_ENTRIES_PATH "/v2/starred_entries.json"
#define FEEDBIN_TAGGINGS_PATH "/v2/taggings.json"
#define FEEDBIN_SUBSCRIPTIONS_PATH "/v2/subscriptions.json"
#define FEEDBIN_UNREAD_ENTRIES_PATH "/v2/unread_entries.json"

using json = nlohmann::json;

namespace Newsboat {

FeedbinApi::FeedbinApi(ConfigContainer& c) : RemoteApi(c)
{
	const std::string http_auth_method = cfg.get_configvalue("http-auth-method");
	if (http_auth_method == "any") {
		// default to basic HTTP auth to prevent Newsboat from doubling up on HTTP
		// requests, since it doesn't "guess" the correct auth type on the first
		// try.
		cfg.set_configvalue("http-auth-method", "basic");
	}
}

bool FeedbinApi::authenticate()
{
	// error check handled in Controller
	const Credentials creds = get_credentials("feedbin", "");
	auth_info = strprintf::fmt("%s:%s", creds.user, creds.pass);

	CurlHandle handle;
	long response_code = 0;
	run_op(FEEDBIN_AUTHENTICATION_PATH, json(), handle);
	curl_easy_getinfo(handle.ptr(), CURLINFO_RESPONSE_CODE, &response_code);

	return response_code == 200;
}

TaggedFeedUrl FeedbinApi::feed_from_json(const json& jfeed,
	const std::vector<std::string>& addtags)
{
	const int feed_id = jfeed["feed_id"];
	const std::string feed_title = jfeed["title"];
	const std::string feed_url = jfeed["feed_url"];

	std::vector<std::string> tags;
	// automatically tag by feedtitle
	tags.push_back(std::string("~") + feed_title);

	// add additional tags
	tags.insert(tags.end(), addtags.cbegin(), addtags.cend());

	auto url = strprintf::fmt("%s#%d", feed_url, feed_id);
	return TaggedFeedUrl(url, tags);
}

std::vector<TaggedFeedUrl> FeedbinApi::get_subscribed_urls()
{
	std::vector<TaggedFeedUrl> feeds;

	const json taggings = run_op(FEEDBIN_TAGGINGS_PATH, json());
	if (taggings.is_null()) {
		LOG(Level::ERROR, "FeedbinApi::get_subscribed_urls: Failed to "
			"retrieve taggings");
		return feeds;
	}

	const json feed_list = run_op(FEEDBIN_SUBSCRIPTIONS_PATH, json());
	if (feed_list.is_null()) {
		LOG(Level::ERROR, "FeedbinApi::get_subscribed_urls: Failed to "
			"retrieve feedlist");
		return feeds;
	}

	for (const json& feed : feed_list) {
		const int feed_id = feed["feed_id"];

		std::vector<std::string> tags;
		for (const auto& tagging : taggings) {
			if (tagging["feed_id"] != feed_id) {
				continue;
			}

			const std::string tag_name = tagging["name"];
			tags.push_back(tagging["name"]);
		}

		feeds.push_back(feed_from_json(feed, tags));
	}

	return feeds;
}

bool FeedbinApi::mark_entries_read(const std::vector<std::string>& ids,
	bool read)
{
	CurlHandle handle;
	long response_code = 0;
	HTTPMethod method = read ? HTTPMethod::DELETE : HTTPMethod::POST;
	json body;
	body["unread_entries"] = ids;
	run_op(FEEDBIN_UNREAD_ENTRIES_PATH, body, handle, method);
	curl_easy_getinfo(handle.ptr(), CURLINFO_RESPONSE_CODE, &response_code);

	return response_code == 200;
}

bool FeedbinApi::mark_all_read(const std::string& combined_feed_url)
{
	std::string feed_id;
	const std::string::size_type pound = combined_feed_url.find_first_of('#');
	if (pound != std::string::npos) {
		feed_id = combined_feed_url.substr(pound + 1);
	} else {
		LOG(Level::ERROR, "FeedbinApi::mark_all_read: Failed to "
			"get feed ID from URL: %s", combined_feed_url);
		return false;
	}

	const std::string feed_entries_query =
		strprintf::fmt("/v2/feeds/%s/entries.json", feed_id);
	const json entries = run_op(feed_entries_query, json());

	std::vector<std::string> entry_ids;
	for (const auto& entry : entries) {
		uint64_t entry_id = entry["id"];
		entry_ids.push_back(std::to_string(entry_id));
	}

	return mark_entries_read(entry_ids, true);
}

bool FeedbinApi::mark_article_read(const std::string& guid, bool read)
{
	std::vector<std::string> entry_ids;
	entry_ids.push_back(guid);
	return mark_entries_read(entry_ids, read);
}

rsspp::Feed FeedbinApi::fetch_feed(const std::string& id,
	CurlHandle& cached_handle)
{
	rsspp::Feed feed;
	feed.rss_version = rsspp::Feed::FEEDBIN_JSON;

	const json unread_entry_ids = run_op(FEEDBIN_UNREAD_ENTRIES_PATH, json());
	if (!unread_entry_ids.is_array()) {
		LOG(Level::ERROR,
			"FeedbinApi::fetch_feed: unread_entry_ids is not an array");
		return feed;
	}

	std::map<int, bool> unread_entry_id_map;
	for (int entry_id : unread_entry_ids) {
		unread_entry_id_map[entry_id] = true;
	}

	const std::string query = strprintf::fmt("/v2/feeds/%s/entries.json", id);

	const json entries = run_op(query, json(), cached_handle, HTTPMethod::GET);
	if (!entries.is_array()) {
		LOG(Level::ERROR, "FeedbinApi::fetch_feed: entries is not an array");
		return feed;
	}

	LOG(Level::INFO, "FeedbinApi::fetch_feed: %" PRIu64 " items",
		static_cast<uint64_t>(entries.size()));
	try {
		for (const auto& entry : entries) {
			rsspp::Item item;

			if (!entry["title"].is_null()) {
				item.title = entry["title"];
			}

			if (!entry["url"].is_null()) {
				item.link = entry["url"];
			}

			if (!entry["author"].is_null()) {
				item.author = entry["author"];
			}

			if (!entry["content"].is_null()) {
				item.content_encoded = entry["content"];
			}

			const int64_t entry_id = entry["id"];
			item.guid = std::to_string(entry_id);

			item.pubDate = entry["published"];

			if (unread_entry_id_map[entry_id]) {
				item.labels.push_back("feedbin:unread");
			} else {
				item.labels.push_back("feedbin:read");
			}

			feed.items.push_back(item);
		}
	} catch (json::exception& e) {
		LOG(Level::ERROR, "Exception occurred while parsing feed: ", e.what());
	}

	std::sort(feed.items.begin(), feed.items.end(),
	[](const rsspp::Item &a, const rsspp::Item &b) {
		return a.pubDate_ts > b.pubDate_ts;
	});

	return feed;
}

bool FeedbinApi::update_article_flags(const std::string& oldflags,
	const std::string& newflags,
	const std::string& guid)
{
	std::string star_flag = cfg.get_configvalue("feedbin-flag-star");
	bool success = true;

	if (!star_flag.empty()) {
		update_flag(oldflags, newflags, star_flag[0],
		[&](bool added) {
			success = star_article(guid, added);
		});
	}

	return success;
}

void FeedbinApi::add_custom_headers(curl_slist** /* custom_headers */)
{
	// nothing required
}

bool FeedbinApi::star_article(const std::string& article_id, bool star)
{
	std::vector<std::string> ids;
	ids.push_back(article_id);
	json body;
	body["starred_entries"] = ids;

	CurlHandle handle;

	HTTPMethod method = star ? HTTPMethod::POST : HTTPMethod::DELETE;

	run_op(FEEDBIN_STARRED_ENTRIES_PATH, body, handle, method);

	long response_code = 0;
	curl_easy_getinfo(handle.ptr(), CURLINFO_RESPONSE_CODE, &response_code);

	return response_code == 200;
}

json FeedbinApi::run_op(const std::string& path, const json& args,
	const HTTPMethod method /* = GET */)
{
	CurlHandle handle;
	return run_op(path, args, handle, method);
}

json FeedbinApi::run_op(const std::string& path, const json& args,
	CurlHandle& easyhandle,
	const HTTPMethod method /* = GET */)
{
	if (method == HTTPMethod::POST || method == HTTPMethod::DELETE) {
		curl_slist* headers = NULL;
		headers = curl_slist_append(headers, "Content-Type: application/json");
		curl_easy_setopt(easyhandle.ptr(), CURLOPT_HTTPHEADER, headers);
	}

	// follow redirects and keep the same request type
	curl_easy_setopt(easyhandle.ptr(), CURLOPT_FOLLOWLOCATION, 1);
	curl_easy_setopt(easyhandle.ptr(), CURLOPT_POSTREDIR, CURL_REDIR_POST_ALL);

	const std::string server = cfg.get_configvalue("feedbin-url");
	const std::string url = server + path;

	std::string* body = nullptr;
	std::string arg_dump;
	if (!args.empty()) {
		arg_dump = args.dump();
		body = &arg_dump;
	}

	const std::string result =
		utils::retrieve_url(url, easyhandle, cfg, auth_info, body, method);

	LOG(Level::INFO, "Feedbin::run_op(%s %s,...): body=%s reply = %s",
		utils::http_method_str(method), path, arg_dump, result);

	json content;
	if (!result.empty()) {
		try {
			content = json::parse(result);
		} catch (json::parse_error& e) {
			LOG(Level::ERROR, "Feedbin::run_op: reply failed to parse: %s", result);
			content = json(nullptr);
		}
	}

	return content;
}

} // namespace Newsboat

#include "minifluxapi.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <curl/curl.h>
#include <json-c/json.h>
#include <memory>
#include <time.h>
#include <thread>

#include "3rd-party/json.hpp"
#include "logger.h"
#include "remoteapi.h"
#include "rss/feed.h"
#include "strprintf.h"
#include "utils.h"

using json = nlohmann::json;

namespace newsboat {

MinifluxApi::MinifluxApi(ConfigContainer* c)
	: RemoteApi(c)
{
	server = cfg->get_configvalue("miniflux-url");

	if (server.empty())
		LOG(Level::CRITICAL,
			"MinifluxApi::MinifluxApi: No Miniflux server configured");
}

MinifluxApi::~MinifluxApi() {}

bool MinifluxApi::authenticate()
{
	auth_info = "";

	Credentials creds = get_credentials("miniflux", "");
	if (creds.user.empty() || creds.pass.empty()) {
		LOG(Level::CRITICAL,
			"Miniflux::retrieve_auth: No user and/or password configured");
	} else {
		auth_info = strprintf::fmt("%s:%s", creds.user, creds.pass);
	}
	return true;
}

TaggedFeedUrl MinifluxApi::feed_from_json(const json& jfeed,
	const std::vector<std::string>& addtags)
{
	const int feed_id = jfeed["id"];
	const std::string str_feed_id = std::to_string(feed_id);
	const std::string feed_title = jfeed["title"];
	const std::string feed_url = jfeed["feed_url"];

	std::vector<std::string> tags;
	// automatically tag by feedtitle
	tags.push_back(std::string("~") + feed_title);

	// add additional tags
	tags.insert(tags.end(), addtags.cbegin(), addtags.cend());

	return TaggedFeedUrl(str_feed_id, tags);
}

std::vector<TaggedFeedUrl> MinifluxApi::get_subscribed_urls()
{
	std::vector<TaggedFeedUrl> feeds;

	json categories = run_op("/v1/categories", json());
	std::map<int, std::string> category_names;
	for (const auto& category : categories) {
		std::string name = category["title"];
		int id = category["id"];
		category_names[id] = name;
	}

	json feed_list = run_op("/v1/feeds", json());
	if (feed_list.is_null()) {
		LOG(Level::ERROR,
			"MinifluxApi::get_subscribed_urls: Failed to "
			"retrieve feedlist");
		return feeds;
	}

	for (const json& feed : feed_list) {
		const int category_id = feed["category"]["id"];
		std::vector<std::string> tags;
		if (category_id > 0) {
			tags.push_back(category_names[category_id]);
		}
		feeds.push_back(feed_from_json(feed, tags));
	}

	return feeds;
}

bool MinifluxApi::mark_all_read(const std::string& id)
{
	// TODO create Miniflux PR to add endpoint for marking all entries in feed
	// as read
	rsspp::Feed feed = fetch_feed(id, nullptr);

	std::vector<std::string> guids;
	for (const auto& item : feed.items) {
		guids.push_back(item.guid);
	}

	json args;
	args["status"] = "read";

	return update_articles(guids, args);
}

bool MinifluxApi::mark_article_read(const std::string& guid, bool read)
{
	// Do this in a thread, as we don't care about the result enough to wait
	// for it.
	std::thread t{[=]()
	{
		LOG(Level::DEBUG,
			"MinifluxApi::mark_article_read: inside thread, marking "
			"article as read...");

		// Call the MinifluxApi's update_article function as a thread.
		json args;
		args["status"] = read ? "read" : "unread";
		this->update_article(guid, args);
	}};
	t.detach();
	return true;
}

bool MinifluxApi::flag_changed(const std::string& oldflags,
	const std::string& newflags,
	const std::string& flagstr)
{
	if (flagstr.length() == 0) {
		return false;
	}

	const char flag = flagstr[0];
	const char* oldptr = strchr(oldflags.c_str(), flag);
	const char* newptr = strchr(newflags.c_str(), flag);

	if (oldptr == nullptr && newptr != nullptr) {
		return true;
	}
	if (oldptr != nullptr && newptr == nullptr) {
		return true;
	}

	return false;
}

bool MinifluxApi::update_article_flags(const std::string& oldflags,
	const std::string& newflags,
	const std::string& guid)
{
	std::string star_flag = cfg->get_configvalue("miniflux-flag-star");
	bool starred_flag_changed = flag_changed(oldflags, newflags, star_flag);

	bool success = true;
	if (starred_flag_changed) {
		success = toggle_star_article(guid);
	}

	return success;
}

rsspp::Feed MinifluxApi::fetch_feed(const std::string& id, CURL* cached_handle)
{
	rsspp::Feed feed;
	feed.rss_version = rsspp::Feed::MINIFLUX_JSON;

	std::string query =
		strprintf::fmt("/v1/feeds/%s/entries?order=published_at&direction=desc", id);

	json content = run_op(query, json(), "GET", cached_handle);
	if (content.is_null()) {
		return feed;
	}

	json entries = content["entries"];
	if (!entries.is_array()) {
		LOG(Level::ERROR,
			"MinifluxApi::fetch_feed: items is not an array");
		return feed;
	}

	LOG(Level::DEBUG,
		"MinifluxApi::fetch_feed: %" PRIu64 " items",
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

			int entry_id = entry["id"];
			item.guid = std::to_string(entry_id);

			item.pubDate = entry["published_at"];

			std::string status = entry["status"];
			if (status == "unread") {
				item.labels.push_back("miniflux:unread");
			} else {
				item.labels.push_back("miniflux:read");
			}

			feed.items.push_back(item);
		}
	} catch (json::exception& e) {
		LOG(Level::ERROR,
			"Exception occurred while parsing feeed: ",
			e.what());
	}

	std::sort(feed.items.begin(),
		feed.items.end(),
	[](const rsspp::Item& a, const rsspp::Item& b) {
		return a.pubDate_ts > b.pubDate_ts;
	});

	return feed;
}

void MinifluxApi::add_custom_headers(curl_slist** /* custom_headers */)
{
	// nothing required
}

json MinifluxApi::run_op(const std::string& path,
	const json& args,
	const std::string& method, /* = GET */
	CURL* cached_handle /* = nullptr */)
{
	std::string url = server + path;

	std::string* body = nullptr;
	std::string arg_dump;
	if (!args.empty()) {
		arg_dump = args.dump();
		body = &arg_dump;
	}

	std::string result = utils::retrieve_url(
			url, cfg, auth_info, body, method, cached_handle);

	LOG(Level::DEBUG,
		"MinifluxApi::run_op(%s,...): post=%s reply = %s",
		path,
		arg_dump,
		result);

	json content;
	if (result.length() > 0) {
		try {
			content = json::parse(result);
		} catch (json::parse_error& e) {
			LOG(Level::ERROR,
				"MinifluxApi::run_op: reply failed to parse: %s",
				result);
			return json(nullptr);
		}
	}

	return content;
}

bool MinifluxApi::toggle_star_article(const std::string& guid)
{
	std::string query = strprintf::fmt("/v1/entries/%s/bookmark", guid);

	json content = run_op(query, json(), "PUT");
	return content.is_null();
}

bool MinifluxApi::update_articles(const std::vector<std::string> guids,
	json& args)
{
	std::vector<int> entry_ids;
	for (const std::string& guid : guids) {
		entry_ids.push_back(std::stoi(guid));
	}

	args["entry_ids"] = entry_ids;
	json content = run_op("/v1/entries", args, "PUT");

	return content.is_null();
}

bool MinifluxApi::update_article(const std::string& guid,
	json& args)
{
	std::vector<std::string> guids;
	guids.push_back(guid);

	return update_articles(guids, args);
}

} // namespace newsboat

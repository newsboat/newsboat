#include "minifluxapi.h"

#include <cinttypes>
#include <curl/curl.h>
#include <thread>

#include "3rd-party/json.hpp"
#include "logger.h"
#include "remoteapi.h"
#include "rss/feed.h"
#include "strprintf.h"
#include "utils.h"

using json = nlohmann::json;
using HTTPMethod = newsboat::utils::HTTPMethod;

namespace newsboat {
MinifluxApi::MinifluxApi(ConfigContainer* c)
	: RemoteApi(c)
{
	server = cfg->get_configvalue("miniflux-url");
	const std::string http_auth_method = cfg->get_configvalue("http-auth-method");
	if (http_auth_method == "any") {
		// default to basic HTTP auth to prevent Newsboat from doubling up on HTTP
		// requests, since it doesn't "guess" the correct auth type on the first
		// try.
		cfg->set_configvalue("http-auth-method", "basic");
	}
}

MinifluxApi::~MinifluxApi() {}

bool MinifluxApi::authenticate()
{
	// error check handled in Controller
	const Credentials creds = get_credentials("miniflux", "");
	auth_info = strprintf::fmt("%s:%s", creds.user, creds.pass);
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

	const json categories = run_op("/v1/categories", json());
	std::map<int, std::string> category_names;
	for (const auto& category : categories) {
		const std::string name = category["title"];
		const int id = category["id"];
		category_names[id] = name;
	}

	const json feed_list = run_op("/v1/feeds", json());
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
	const rsspp::Feed feed = fetch_feed(id, nullptr);

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

bool MinifluxApi::update_article_flags(const std::string& /* oldflags */,
	const std::string& /* newflags */,
	const std::string& /* guid */)
{
	return false;
}

rsspp::Feed MinifluxApi::fetch_feed(const std::string& id, CURL* cached_handle)
{
	rsspp::Feed feed;
	feed.rss_version = rsspp::Feed::MINIFLUX_JSON;

	const std::string query =
		strprintf::fmt("/v1/feeds/%s/entries?order=published_at&direction=desc", id);

	const json content = run_op(query, json(), HTTPMethod::GET, cached_handle);
	if (content.is_null()) {
		return feed;
	}

	const json entries = content["entries"];
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

			const int entry_id = entry["id"];
			item.guid = std::to_string(entry_id);

			item.pubDate = entry["published_at"];

			const std::string status = entry["status"];
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
	const HTTPMethod method, /* = GET */
	CURL* cached_handle /* = nullptr */)
{
	const std::string url = server + path;

	std::string* body = nullptr;
	std::string arg_dump;
	if (!args.empty()) {
		arg_dump = args.dump();
		body = &arg_dump;
	}

	const std::string result = utils::retrieve_url(
			url, cfg, auth_info, body, method, cached_handle);

	LOG(Level::DEBUG,
		"MinifluxApi::run_op(%s %s,...): body=%s reply = %s",
		utils::http_method_str(method),
		path,
		arg_dump,
		result);

	json content;
	if (!result.empty()) {
		try {
			content = json::parse(result);
		} catch (json::parse_error& e) {
			LOG(Level::ERROR,
				"MinifluxApi::run_op: reply failed to parse: %s",
				result);
			content = json(nullptr);
		}
	}

	return content;
}

bool MinifluxApi::update_articles(const std::vector<std::string> guids,
	json& args)
{
	std::vector<int> entry_ids;
	for (const std::string& guid : guids) {
		entry_ids.push_back(std::stoi(guid));
	}

	args["entry_ids"] = entry_ids;
	const json content = run_op("/v1/entries", args, HTTPMethod::PUT);

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

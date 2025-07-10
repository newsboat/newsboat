#include "minifluxapi.h"

#include <cinttypes>
#include <curl/curl.h>
#include <iostream>
#include <thread>

#include "3rd-party/json.hpp"
#include "config.h"
#include "curlhandle.h"
#include "logger.h"
#include "remoteapi.h"
#include "rss/feed.h"
#include "strprintf.h"
#include "utils.h"

using json = nlohmann::json;
using HTTPMethod = Newsboat::utils::HTTPMethod;

namespace Newsboat {
MinifluxApi::MinifluxApi(ConfigContainer& c)
	: RemoteApi(c)
{
	server = cfg.get_configvalue("miniflux-url");
	const std::string http_auth_method = cfg.get_configvalue("http-auth-method");
	if (http_auth_method == "any") {
		// default to basic HTTP auth to prevent Newsboat from doubling up on HTTP
		// requests, since it doesn't "guess" the correct auth type on the first
		// try.
		cfg.set_configvalue("http-auth-method", "basic");
	}
}

bool MinifluxApi::authenticate()
{
	// error check handled in Controller
	const Credentials creds = get_credentials("miniflux", "");
	if (!creds.token.empty()) {
		auth_token = creds.token;
		auth_info = "";
	} else {
		auth_token = "";
		auth_info = strprintf::fmt("%s:%s", creds.user, creds.pass);
	}

	CurlHandle handle;
	long response_code = 0;
	const std::string path = "/v1/me";
	run_op(path, json(), handle);
	curl_easy_getinfo(handle.ptr(), CURLINFO_RESPONSE_CODE, &response_code);

	if (response_code == 401) {
		return false;
	}
	if (response_code < 200 || response_code > 299) {
		const auto url = server + path;
		std::cerr << strprintf::fmt(
				_("Authentication check using %s failed (HTTP response code: %s)"),
				url,
				std::to_string(response_code)) << std::endl;
		return false;
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
	CurlHandle easyHandle;
	const rsspp::Feed feed = fetch_feed(id, easyHandle);

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

bool MinifluxApi::star_article(const std::string& guid, bool star)
{
	std::string getstate;
	getstate = strprintf::fmt(
			"/v1/entries/%s",
			guid);
	const json status = run_op(getstate, json(), HTTPMethod::GET);
	bool current_star = status["starred"];

	if (star != current_star) {
		std::string putcontent;
		putcontent = strprintf::fmt(
				"/v1/entries/%s/bookmark",
				guid);
		const json content = run_op(putcontent, json(), HTTPMethod::PUT);
		return content.is_null();
	} else {
		return true;
	}
}

bool MinifluxApi::update_article_flags(const std::string&  oldflags,
	const std::string&  newflags,
	const std::string&  guid )
{
	std::string star_flag = cfg.get_configvalue("miniflux-flag-star");
	bool success = true;

	if (star_flag.length() > 0) {
		update_flag(oldflags, newflags, star_flag[0], [&](bool added) {
			success = star_article(guid, added);
		});
	}

	return success;
}

rsspp::Feed MinifluxApi::fetch_feed(const std::string& id, CurlHandle& cached_handle)
{
	rsspp::Feed feed;
	feed.rss_version = rsspp::Feed::MINIFLUX_JSON;

	const std::string query =
		id == "starred" ?
		"/v1/entries?starred=true"
		: strprintf::fmt("/v1/feeds/%s/entries?order=published_at&direction=desc&limit=%u",
			id,
			cfg.get_configvalue_as_int("miniflux-min-items"));

	const json content = run_op(query, json(), cached_handle, HTTPMethod::GET);
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

			if (!entry["enclosures"].is_null() && entry["enclosures"].is_array()) {
				for (const auto& enclosure : entry["enclosures"]) {
					if (!enclosure["url"].is_null() && !enclosure["mime_type"].is_null()) {
						rsspp::Enclosure enc;
						enc.url = enclosure["url"];
						enc.type = enclosure["mime_type"];
						item.enclosures.push_back(enc);
					}
				}
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
	const HTTPMethod method /* = GET */)
{
	CurlHandle handle;
	return run_op(path, args, handle, method);
}

json MinifluxApi::run_op(const std::string& path,
	const json& args,
	CurlHandle& easyhandle,
	const HTTPMethod method /* = GET */)
{
	// follow redirects and keep the same request type
	curl_easy_setopt(easyhandle.ptr(), CURLOPT_FOLLOWLOCATION, 1);
	curl_easy_setopt(easyhandle.ptr(), CURLOPT_POSTREDIR, CURL_REDIR_POST_ALL);

	if (!auth_token.empty()) {
		std::string header = "X-Auth-Token: " + auth_token;

		curl_slist* headers = NULL;
		headers = curl_slist_append(headers, header.c_str());
		curl_easy_setopt(easyhandle.ptr(), CURLOPT_HTTPHEADER, headers);
	}

	const std::string url = server + path;

	std::string* body = nullptr;
	std::string arg_dump;
	if (!args.empty()) {
		arg_dump = args.dump();
		body = &arg_dump;
	}

	const std::string result = utils::retrieve_url(
			url, easyhandle, cfg, auth_info, body, method);

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

} // namespace Newsboat

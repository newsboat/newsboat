#include "ttrssapi.h"

#include <algorithm>
#include <cinttypes>
#include <thread>
#include <time.h>

#include "3rd-party/json.hpp"
#include "curlhandle.h"
#include "logger.h"
#include "remoteapi.h"
#include "rss/feed.h"
#include "strprintf.h"
#include "utils.h"

using json = nlohmann::json;

namespace Newsboat {

TtRssApi::TtRssApi(ConfigContainer& c)
	: RemoteApi(c)
{
	single = (cfg.get_configvalue("ttrss-mode") == "single");
	if (single) {
		auth_info = strprintf::fmt("%s:%s",
				cfg.get_configvalue("ttrss-login"),
				cfg.get_configvalue("ttrss-password"));
	} else {
		auth_info = "";
	}
	sid = "";
}

bool TtRssApi::authenticate()
{
	if (auth_lock.try_lock()) {
		sid = retrieve_sid();
		auth_lock.unlock();
	} else {
		// wait for other thread to finish and return its result:
		auth_lock.lock();
		auth_lock.unlock();
	}

	return sid != "";
}

std::string TtRssApi::retrieve_sid()
{
	std::map<std::string, std::string> args;

	Credentials cred = get_credentials("ttrss", "Tiny Tiny RSS");
	if (cred.user.empty() || cred.pass.empty()) {
		return "";
	}

	args["user"] = single ? "admin" : cred.user;
	args["password"] = cred.pass;
	if (single) {
		auth_info = strprintf::fmt("%s:%s", cred.user, cred.pass);
	} else {
		auth_info = "";
	}
	json content = run_op("login", args);

	if (content.is_null()) {
		return "";
	}

	std::string sid;
	try {
		sid = content["session_id"];
	} catch (json::exception& e) {
		LOG(Level::INFO,
			"TtRssApi::retrieve_sid: couldn't extract session_id: "
			"%s",
			e.what());
	}

	try {
		api_level = content["api_level"];
	} catch (json::exception& e) {
		LOG(Level::INFO,
			"TtRssApi::retrieve_sid: couldn't determine api_level "
			"from response: %s",
			e.what());
	}

	LOG(Level::DEBUG, "TtRssApi::retrieve_sid: sid = '%s'", sid);

	return sid;
}

unsigned int TtRssApi::query_api_level()
{
	if (api_level == -1) {
		// api level was never queried. Do it now.
		std::map<std::string, std::string> args;
		json content = run_op("getApiLevel", args);

		try {
			api_level = content["level"];
			LOG(Level::DEBUG,
				"TtRssApi::query_api_level: determined level: "
				"%d",
				api_level);
		} catch (json::exception& e) {
			// From
			// https://git.tt-rss.org/git/tt-rss/wiki/ApiReference
			// "Whether tt-rss returns error for this method (e.g.
			//  version:1.5.7 and below) client should assume API
			//  level 0."
			LOG(Level::DEBUG,
				"TtRssApi::query_api_level: failed to "
				"determine "
				"level, assuming 0");
			api_level = 0;
		}
	}
	return api_level;
}

json TtRssApi::run_op(const std::string& op,
	const std::map<std::string, std::string>& args,
	bool try_login /* = true */)
{
	CurlHandle handle;
	return TtRssApi::run_op(op, args, handle, try_login);
}

json TtRssApi::run_op(const std::string& op,
	const std::map<std::string, std::string>& args,
	CurlHandle& cached_handle,
	bool try_login /* = true */)
{
	std::string url =
		strprintf::fmt("%s/api/", cfg.get_configvalue("ttrss-url"));

	// First build the request payload
	std::string req_data;
	{
		json requestparam;

		requestparam["op"] = op;
		if (!sid.empty()) {
			requestparam["sid"] = sid;
		}

		// Note: We are violating the upstream-api's types here by
		// packing all information
		//       into strings. If things start to break, this would be a
		//       good place to start.
		for (const auto& arg : args) {
			requestparam[arg.first] = arg.second;
		}

		req_data = requestparam.dump();
	}

	std::string result = utils::retrieve_url(
			url, cached_handle, cfg, auth_info, &req_data, utils::HTTPMethod::POST);

	LOG(Level::DEBUG,
		"TtRssApi::run_op(%s,...): post=%s reply = %s",
		op,
		req_data,
		result);

	json reply;
	try {
		reply = json::parse(result);
	} catch (json::parse_error& e) {
		LOG(Level::ERROR,
			"TtRssApi::run_op: reply failed to parse: %s",
			result);
		return json(nullptr);
	}

	int status;
	try {
		status = reply.at("status");
	} catch (json::exception& e) {
		LOG(Level::ERROR,
			"TtRssApi::run_op: no status code: %s",
			e.what());
		return json(nullptr);
	}

	json content;
	try {
		content = reply.at("content");
	} catch (json::exception& e) {
		LOG(Level::ERROR,
			"TtRssApi::run_op: no content part in answer from "
			"server");
		return json(nullptr);
	}

	if (status != 0) {
		if (content["error"] == "NOT_LOGGED_IN" && try_login) {
			if (authenticate()) {
				return run_op(op, args, cached_handle, false);
			} else {
				return json(nullptr);
			}
		} else {
			LOG(Level::ERROR,
				"TtRssApi::run_op: status: %d, error: '%s'",
				status,
				content["error"].dump());
			return json(nullptr);
		}
	}

	return content;
}

TaggedFeedUrl TtRssApi::feed_from_json(const json& jfeed,
	const std::vector<std::string>& addtags)
{
	const int feed_id = jfeed["id"];
	const std::string feed_title = jfeed["title"];
	const std::string feed_url = jfeed["feed_url"];

	std::vector<std::string> tags;
	// automatically tag by feedtitle
	tags.push_back(std::string("~") + feed_title);

	// add additional tags
	tags.insert(tags.end(), addtags.cbegin(), addtags.cend());

	auto url = strprintf::fmt("%s#%d", feed_url, feed_id);
	return TaggedFeedUrl(url, tags);
	// TODO: cache feed_id -> feed_url (or feed_url -> feed_id ?)
}

int TtRssApi::parse_category_id(const json& jcatid)
{
	int cat_id;
	// TTRSS (commit "b0113adac42383b8039eb92ccf3ee2ec0ee70346") returns a
	// string for regular items and an integer for predefined categories
	// (like -1)
	if (jcatid.is_string()) {
		cat_id = std::stoi(jcatid.get<std::string>());
	} else {
		cat_id = jcatid;
	}
	return cat_id;
}

std::vector<TaggedFeedUrl> TtRssApi::get_subscribed_urls()
{
	std::vector<TaggedFeedUrl> feeds;

	json categories =
		run_op("getCategories", std::map<std::string, std::string>());

	if (query_api_level() >= 2) {
		// getFeeds with cat_id -3 since 1.5.0, so at least since
		// api-level 2
		std::map<int, std::string> category_names;
		for (const auto& cat : categories) {
			std::string cat_name = cat["title"];
			int cat_id = parse_category_id(cat["id"]);
			category_names[cat_id] = cat_name;
		}

		std::map<std::string, std::string> args;
		// All feeds, excluding virtual feeds (e.g. Labels and such)
		args["cat_id"] = "-3";
		json feedlist = run_op("getFeeds", args);

		if (feedlist.is_null()) {
			LOG(Level::ERROR,
				"TtRssApi::get_subscribed_urls: Failed to "
				"retrieve feedlist");
			return feeds;
		}

		for (json& feed : feedlist) {
			const int cat_id = feed["cat_id"];
			std::vector<std::string> tags;
			if (cat_id > 0) {
				tags.push_back(category_names[cat_id]);
			}
			feeds.push_back(feed_from_json(feed, tags));
		}

	} else {
		try {
			// first fetch feeds within no category
			fetch_feeds_per_category(json(nullptr), feeds);

			// then fetch the feeds of all categories
			for (const auto& i : categories) {
				fetch_feeds_per_category(i, feeds);
			}
		} catch (json::exception& e) {
			LOG(Level::ERROR,
				"TtRssApi::get_subscribed_urls:"
				" Failed to determine subscribed urls: %s",
				e.what());
			return std::vector<TaggedFeedUrl>();
		}
	}

	return feeds;
}

void TtRssApi::add_custom_headers(curl_slist** /* custom_headers */)
{
	// nothing required
}

bool TtRssApi::mark_all_read(const std::string& feed_url)
{
	std::map<std::string, std::string> args;
	args["feed_id"] = url_to_id(feed_url);
	json content = run_op("catchupFeed", args);

	return !content.is_null();
}

bool TtRssApi::mark_article_read(const std::string& guid, bool read)
{
	// Do this in a thread, as we don't care about the result enough to wait
	// for it.
	std::thread t{[=]()
	{
		LOG(Level::DEBUG,
			"TtRssApi::mark_article_read: inside thread, marking "
			"thread as read...");

		// Call the TtRssApi's update_article function as a thread.
		this->update_article(guid, 2, read ? 0 : 1);
	}};
	t.detach();
	return true;
}

bool TtRssApi::update_article_flags(const std::string& oldflags,
	const std::string& newflags,
	const std::string& guid)
{
	std::string star_flag = cfg.get_configvalue("ttrss-flag-star");
	std::string publish_flag = cfg.get_configvalue("ttrss-flag-publish");
	bool success = true;

	if (star_flag.length() > 0) {
		update_flag(oldflags, newflags, star_flag[0], [&](bool added) {
			success = star_article(guid, added);
		});
	}

	if (publish_flag.length() > 0) {
		update_flag(oldflags, newflags, publish_flag[0], [&](bool added) {
			success = publish_article(guid, added);
		});
	}

	return success;
}

rsspp::Feed TtRssApi::fetch_feed(const std::string& id, CurlHandle& cached_handle)
{
	rsspp::Feed f;

	f.rss_version = rsspp::Feed::TTRSS_JSON;

	std::map<std::string, std::string> args;
	args["feed_id"] = id;
	args["show_content"] = "1";
	args["include_attachments"] = "1";
	json content = run_op("getHeadlines", args, cached_handle, true);

	if (content.is_null()) {
		return f;
	}

	if (!content.is_array()) {
		LOG(Level::ERROR,
			"TtRssApi::fetch_feed: content is not an array");
		return f;
	}

	LOG(Level::DEBUG,
		"TtRssApi::fetch_feed: %" PRIu64 " items",
		static_cast<uint64_t>(content.size()));

	try {
		for (const auto& item_obj : content) {
			rsspp::Item item;

			if (!item_obj["title"].is_null()) {
				item.title = item_obj["title"];
			}

			if (!item_obj["link"].is_null()) {
				item.link = item_obj["link"];
			}

			if (!item_obj["author"].is_null()) {
				item.author = item_obj["author"];
			}

			if (!item_obj["content"].is_null()) {
				item.content_encoded = item_obj["content"];
			}

			if (!item_obj["attachments"].is_null()) {
				for (const json& a : item_obj["attachments"]) {
					if (!a["content_url"].is_null() && !a["content_type"].is_null()) {
						item.enclosures.push_back(
						rsspp::Enclosure {
							a["content_url"],
							a["content_type"],
							"",
							"",
						}
						);
						break;
					}
				}
			}

			int id = item_obj["id"];
			item.guid = strprintf::fmt("%d", id);

			bool unread = item_obj["unread"];
			if (unread) {
				item.labels.push_back("ttrss:unread");
			} else {
				item.labels.push_back("ttrss:read");
			}

			int updated_time = item_obj["updated"];
			time_t updated = static_cast<time_t>(updated_time);

			item.pubDate = utils::mt_strf_localtime(
					"%a, %d %b %Y %H:%M:%S %z",
					updated);
			item.pubDate_ts = updated;

			f.items.push_back(item);
		}
	} catch (json::exception& e) {
		LOG(Level::ERROR,
			"Exception occurred while parsing feeed: ",
			e.what());
	}

	std::sort(f.items.begin(),
		f.items.end(),
	[](const rsspp::Item& a, const rsspp::Item& b) {
		return a.pubDate_ts > b.pubDate_ts;
	});

	return f;
}

void TtRssApi::fetch_feeds_per_category(const json& cat,
	std::vector<TaggedFeedUrl>& feeds)
{
	json cat_name;

	if (cat.is_null()) {
		// As uncategorized is a category itself (id = 0) and the
		// default value for a getFeeds is id = 0, the feeds in
		// uncategorized will appear twice
		return;
	}

	std::string cat_id;
	// TTRSS (commit "b0113adac42383b8039eb92ccf3ee2ec0ee70346") returns a
	// string for regular items and an integer for predefined categories
	// (like -1)
	if (cat["id"].is_string()) {
		cat_id = cat["id"];
	} else {
		int i = cat["id"];
		cat_id = std::to_string(i);
	}

	// ignore special categories, for now
	if (std::stoi(cat_id) < 0) {
		return;
	}

	cat_name = cat["title"];
	LOG(Level::DEBUG,
		"TtRssApi::fetch_feeds_per_category: fetching id = %s title = "
		"%s",
		cat_id,
		cat_name.is_null() ? "<null>" : cat_name.get<std::string>());

	std::map<std::string, std::string> args;
	args["cat_id"] = cat_id;

	json feed_list_obj = run_op("getFeeds", args);

	if (feed_list_obj.is_null()) {
		return;
	}

	// Automatically provide the category as a tag
	std::vector<std::string> tags;
	if (!cat_name.is_null()) {
		tags.push_back(cat_name.get<std::string>());
	}

	for (json& feed : feed_list_obj) {
		feeds.push_back(feed_from_json(feed, tags));
	}
}

bool TtRssApi::star_article(const std::string& guid, bool star)
{
	return update_article(guid, 0, star ? 1 : 0);
}

bool TtRssApi::publish_article(const std::string& guid, bool publish)
{
	return update_article(guid, 1, publish ? 1 : 0);
}

bool TtRssApi::update_article(const std::string& guid, int field, int mode)
{
	std::map<std::string, std::string> args;
	args["article_ids"] = guid;
	args["field"] = std::to_string(field);
	args["mode"] = std::to_string(mode);
	json content = run_op("updateArticle", args);

	return !content.is_null();
}

std::string TtRssApi::url_to_id(const std::string& url)
{
	const std::string::size_type pound = url.find_first_of('#');
	if (pound == std::string::npos) {
		return "";
	} else {
		return url.substr(pound + 1);
	}
}

} // namespace Newsboat

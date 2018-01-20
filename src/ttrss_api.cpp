#include <json.hpp>
#include <remote_api.h>
#include <ttrss_api.h>
#include <cstring>
#include <algorithm>
#include <time.h>
#include <thread>

#include <strprintf.h>

using json = nlohmann::json;

namespace newsboat {

ttrss_api::ttrss_api(configcontainer * c) : remote_api(c) {
	single = (cfg->get_configvalue("ttrss-mode") == "single");
	if (single) {
		auth_info = strprintf::fmt("%s:%s", cfg->get_configvalue("ttrss-login"), cfg->get_configvalue("ttrss-password"));
	} else {
		auth_info = "";
	}
	sid = "";
}

ttrss_api::~ttrss_api() {
}

bool ttrss_api::authenticate() {
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

std::string ttrss_api::retrieve_sid() {
	std::map<std::string, std::string> args;

	credentials cred = get_credentials("ttrss", "Tiny Tiny RSS");
	if (cred.user.empty() || cred.pass.empty()) {
		return "";
	}

	args["user"] = single ? "admin" : cred.user.c_str();
	args["password"] = cred.pass.c_str();
	if (single) {
		auth_info = strprintf::fmt("%s:%s", cred.user, cred.pass);
	} else {
		auth_info = "";
	}
	json content = run_op("login", args);

	if (content.is_null())
		return "";

	std::string sid;
	try {
		sid = content["session_id"];
	} catch (json::exception& e) {}

	try {
		api_level = content["api_level"];
	} catch (json::exception& e) {};

	LOG(level::DEBUG, "ttrss_api::retrieve_sid: sid = '%s'", sid);

	return sid;
}

unsigned int ttrss_api::query_api_level()
{
	if (api_level == -1) {
		// api level was never queried. Do it now.
		std::map<std::string, std::string> args;
		json content = run_op("getApiLevel", args);

		try {
			api_level = content["level"];
			LOG(level::DEBUG, "ttrss_api::query_api_level: determined level: %d", api_level);
		} catch (json::exception& e) {
			// From https://git.tt-rss.org/git/tt-rss/wiki/ApiReference
			// "Whether tt-rss returns error for this method (e.g.
			//  version:1.5.7 and below) client should assume API level 0."
			LOG(level::DEBUG, "ttrss_api::query_api_level: failed to determine level, assuming 0");
			api_level = 0;
		}
	}
	return api_level;
}

json ttrss_api::run_op(const std::string& op,
                       const std::map<std::string, std::string >& args,
                       bool try_login)
{
	std::string url = strprintf::fmt("%s/api/", cfg->get_configvalue("ttrss-url"));

	// First build the request payload
	std::string req_data;
	{
		json requestparam;

		requestparam["op"] = op;
		if (!sid.empty()) {
			requestparam["sid"] = sid;
		}

		// Note: We are violating the upstream-api's types here by packing all information
		//       into strings. If things start to break, this would be a good place to start.
		for (auto arg : args) {
			requestparam[arg.first] = arg.second;
		}

		req_data = requestparam.dump();
	}

	std::string result = utils::retrieve_url(url, cfg, auth_info, &req_data);

	LOG(level::DEBUG, "ttrss_api::run_op(%s,...): post=%s reply = %s", op, req_data, result);

	json reply;
	try {
		reply = json::parse(result);
	} catch (json::parse_error& e) {
		LOG(level::ERROR, "ttrss_api::run_op: reply failed to parse: %s", result);
		return json(nullptr);
	}

	int status;
	try {
		status = reply.at("status");
	} catch (json::exception& e) {
		LOG(level::ERROR, "ttrss_api::run_op: no status code: %s", e.what());
		return json(nullptr);
	}

	if (status != 0) {
		if (reply["error"] == "NOT_LOGGED_IN" && try_login) {
			if (authenticate())
				return run_op(op, args, false);
			else
				return json(nullptr);
		} else {
			LOG(level::ERROR, "ttrss_api::run_op: status: %d, error: '%s'", status, reply["error"].dump());
			return json(nullptr);
		}
	}

	json content;
	try {
		content = reply.at("content");
	} catch (json::exception& e) {
		LOG(level::ERROR, "ttrss_api::run_op: no content part in answer from server");
		return json(nullptr);
	}

	return content;
}

std::vector<tagged_feedurl> ttrss_api::get_subscribed_urls() {
	std::vector<tagged_feedurl> feeds;


	json content = run_op("getCategories", std::map<std::string, std::string>());

	try {
		// first fetch feeds within no category
		fetch_feeds_per_category(json(nullptr), feeds);

		// then fetch the feeds of all categories
		for (auto& i : content) {
			fetch_feeds_per_category(i, feeds);
		}
	} catch (json::exception& e) {
		LOG(level::ERROR, "ttrss_api::get_subscribed_urls:"
		                  " Failed to determine subscribed urls: %s", e.what());
		return std::vector<tagged_feedurl>();
	}


	return feeds;
}

void ttrss_api::add_custom_headers(curl_slist** /* custom_headers */) {
	// nothing required
}

bool ttrss_api::mark_all_read(const std::string& feed_url) {

	std::map<std::string, std::string> args;
	args["feed_id"] = url_to_id(feed_url);
	json content = run_op("catchupFeed", args);

	if (content.is_null())
		return false;

	return true;
}

bool ttrss_api::mark_article_read(const std::string& guid, bool read) {
	// Do this in a thread, as we don't care about the result enough to wait for
	// it.
	std::thread t {[=](){
		LOG(level::DEBUG, "ttrss_api::mark_article_read: inside thread, marking thread as read...");

		// Call the ttrss_api's update_article function as a thread.
		this->update_article(guid, 2, read ? 0 : 1 );
	}};
	t.detach();
	return true;
}

bool ttrss_api::update_article_flags(const std::string& oldflags, const std::string& newflags, const std::string& guid) {
	std::string star_flag = cfg->get_configvalue("ttrss-flag-star");
	std::string publish_flag = cfg->get_configvalue("ttrss-flag-publish");
	bool success = true;

	if (star_flag.length() > 0) {
		if (strchr(oldflags.c_str(), star_flag[0])==nullptr && strchr(newflags.c_str(), star_flag[0])!=nullptr) {
			success = star_article(guid, true);
		} else if (strchr(oldflags.c_str(), star_flag[0])!=nullptr && strchr(newflags.c_str(), star_flag[0])==nullptr) {
			success = star_article(guid, false);
		}
	}

	if (publish_flag.length() > 0) {
		if (strchr(oldflags.c_str(), publish_flag[0])==nullptr && strchr(newflags.c_str(), publish_flag[0])!=nullptr) {
			success = publish_article(guid, true);
		} else if (strchr(oldflags.c_str(), publish_flag[0])!=nullptr && strchr(newflags.c_str(), publish_flag[0])==nullptr) {
			success = publish_article(guid, false);
		}
	}

	return success;
}

rsspp::feed ttrss_api::fetch_feed(const std::string& id) {
	rsspp::feed f;

	f.rss_version = rsspp::TTRSS_JSON;

	std::map<std::string, std::string> args;
	args["feed_id"] = id;
	args["show_content"] = "1";
	args["include_attachments"] = "1";
	json content = run_op("getHeadlines", args);

	if (content.is_null())
		return f;

	if (!content.is_array()) {
		LOG(level::ERROR, "ttrss_api::fetch_feed: content is not an array");
		return f;
	}

	LOG(level::DEBUG, "ttrss_api::fetch_feed: %d items", content.size());

	try {
		for (auto &item_obj : content) {
			rsspp::item item;

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

			if (!item_obj["attachments"].is_null())
			{
				if (item_obj["attachements"].size() >= 1) {
					json a = item_obj["attachements"].front();
					if (!a["content_url"].is_null()) {
						item.enclosure_url = a["content_url"];
					}
					if (!a["content_type"].is_null()) {
						item.enclosure_type = a["content_type"];
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
			char rfc822_date[128];
			strftime(rfc822_date, sizeof(rfc822_date), "%a, %d %b %Y %H:%M:%S %z", gmtime(&updated));
			item.pubDate = rfc822_date;
			item.pubDate_ts = updated;

			f.items.push_back(item);
		}
	} catch (json::exception& e) {
		LOG(level::ERROR, "Exception occured while parsing feeed: ", e.what());
	}

	std::sort(f.items.begin(), f.items.end(), [](const rsspp::item& a, const rsspp::item& b) {
		return a.pubDate_ts > b.pubDate_ts;
	});

	return f;
}

void ttrss_api::fetch_feeds_per_category(
    const json &cat, std::vector<tagged_feedurl>& feeds)
{
	json cat_name;

	if (cat.is_null()) {
		// As uncategorized is a category itself (id = 0) and the default value
		// for a getFeeds is id = 0, the feeds in uncategorized will appear twice
		return;
	}

	std::string cat_id;
	// TTRSS (commit "b0113adac42383b8039eb92ccf3ee2ec0ee70346") returns a string
	// for regular items and an integer for predefined categories (like -1)
	if (cat["id"].is_string()) {
		cat_id = cat["id"];
	} else {
		int i = cat["id"];
		cat_id = std::to_string(i);
	}

	// ignore special categories, for now
	if (std::stoi(cat_id) < 0)
		return;

	cat_name = cat["title"];
	LOG(level::DEBUG, "ttrss_api::fetch_feeds_per_category: fetching id = %s title = %s", cat_id
	                , cat_name.is_null() ? "<null>" : cat_name.get<std::string>());

	std::map<std::string, std::string> args;
	args["cat_id"] = cat_id;

	json feed_list_obj = run_op("getFeeds", args);

	if (feed_list_obj.is_null())
		return;

	for (json& feed : feed_list_obj) {
		const int feed_id            = feed["id"];
		const std::string feed_title = feed["title"];
		const std::string feed_url   = feed["feed_url"];

		std::vector<std::string> tags;
		tags.push_back(std::string("~") + feed_title);

		if (!cat_name.is_null()) {
			tags.push_back(cat_name.get<std::string>());
		}

		auto url = strprintf::fmt("%s#%d", feed_url, feed_id);
		feeds.push_back(tagged_feedurl(url, tags));

		// TODO: cache feed_id -> feed_url (or feed_url -> feed_id ?)
	}
}

bool ttrss_api::star_article(const std::string& guid, bool star) {
	return update_article(guid, 0, star ? 1 : 0);
}

bool ttrss_api::publish_article(const std::string& guid, bool publish) {
	return update_article(guid, 1, publish ? 1 : 0);
}

bool ttrss_api::update_article(const std::string& guid, int field, int mode) {

	std::map<std::string, std::string> args;
	args["article_ids"] = guid;
	args["field"] = std::to_string(field);
	args["mode"] = std::to_string(mode);
	json content = run_op("updateArticle", args);

	if (content.is_null())
		return false;

	return true;
}

std::string ttrss_api::url_to_id(const std::string& url) {
	const std::string::size_type pound = url.find_first_of('#');
	if (pound == std::string::npos) {
		return "";
	} else {
		return url.substr(pound+1);
	}
}


}

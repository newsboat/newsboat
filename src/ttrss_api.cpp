#include <json.h>
#include <remote_api.h>
#include <ttrss_api.h>
#include <cstring>
#include <algorithm>
#include <time.h>

#include <wordexp.h>
#include <unistd.h>
#include <iostream>

#include <markreadthread.h>
#include <strprintf.h>

namespace newsbeuter {

ttrss_api::ttrss_api(configcontainer * c) : remote_api(c) {
	single = (cfg->get_configvalue("ttrss-mode") == "single");
	auth_info = strprintf::fmt("%s:%s", cfg->get_configvalue("ttrss-login"), cfg->get_configvalue("ttrss-password"));
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

	std::string user = cfg->get_configvalue("ttrss-login");
	bool flushed = false;
	if (user == "") {
		std::cout << std::endl;
		std::cout.flush();
		flushed = true;
		std::cout << "Username for Tiny Tiny RSS: ";
		std::cin >> user;

		if (user == "") {
			return "";
		}
	}

	std::string pass = cfg->get_configvalue("ttrss-password");
	if (pass == "") {
		wordexp_t exp;
		std::ifstream ifs;
		wordexp(cfg->get_configvalue("ttrss-passwordfile").c_str(),&exp,0);
		if (exp.we_wordc > 0) {
			ifs.open(exp.we_wordv[0]);
		}
		wordfree(&exp);
		if (!ifs.is_open()) {
			if (!flushed) {
				std::cout << std::endl;
				std::cout.flush();
			}
			// Find a way to do this in C++ by removing cin echoing.
			pass = std::string( getpass("Password for Tiny Tiny RSS: ") );

		} else {
			ifs >> pass;
			if (pass == "") {
				return "";
			}
		}
	}

	args["user"] = single ? "admin" : user.c_str();
	args["password"] = pass.c_str();
	auth_info = strprintf::fmt("%s:%s", user, pass);
	json_object * content = run_op("login", args);

	if (content == nullptr)
		return "";

	json_object * session_id {};
	json_object_object_get_ex(content, "session_id", &session_id);
	std::string sid = json_object_get_string(session_id);

	json_object_put(content);

	LOG(level::DEBUG, "ttrss_api::retrieve_sid: sid = '%s'", sid);

	return sid;
}

json_object* ttrss_api::run_op(const std::string& op,
                               const std::map<std::string, std::string >& args,
                               bool try_login)
{
	std::string url = strprintf::fmt("%s/api/", cfg->get_configvalue("ttrss-url"));

	std::string req_data = "{\"op\":\"" + op + "\",\"sid\":\"" + sid + "\"";

	for (auto arg : args) {
		req_data += ",\"" + arg.first + "\":\"" + arg.second + "\"";
	}
	req_data += "}";

	std::string result = utils::retrieve_url(url, cfg, auth_info, &req_data);

	LOG(level::DEBUG, "ttrss_api::run_op(%s,...): post=%s reply = %s", op, req_data, result);

	json_object * reply = json_tokener_parse(result.c_str());
	if (reply == nullptr) {
		LOG(level::ERROR, "ttrss_api::run_op: reply failed to parse: %s", result);
		return nullptr;
	}

	json_object* status {};
	json_object_object_get_ex(reply, "status", &status);
	if (status == nullptr) {
		LOG(level::ERROR, "ttrss_api::run_op: no status code");
		return nullptr;
	}

	json_object* content {};
	json_object_object_get_ex(reply, "content", &content);
	if (content == nullptr) {
		LOG(level::ERROR, "ttrss_api::run_op: no content part in answer from server");
		return nullptr;
	}

	if (json_object_get_int(status) != 0) {
		json_object* error {};
		json_object_object_get_ex(content, "error", &error);
		if ((strcmp(json_object_get_string(error), "NOT_LOGGED_IN") == 0) && try_login) {
			json_object_put(reply);
			if (authenticate())
				return run_op(op, args, false);
			else
				return nullptr;
		} else {
			json_object_put(reply);
			return nullptr;
		}
	}

	// free the parent object, without freeing content as well:
	json_object_get(content);
	json_object_put(reply);
	return content;
}

std::vector<tagged_feedurl> ttrss_api::get_subscribed_urls() {

	std::vector<tagged_feedurl> feeds;

	json_object* content = run_op("getCategories", std::map<std::string, std::string>());
	if (!content)
		return feeds;

	if (json_object_get_type(content) != json_type_array)
		return feeds;

	struct array_list * categories = json_object_get_array(content);

	int catsize = array_list_length(categories);

	// first fetch feeds within no category
	fetch_feeds_per_category(nullptr, feeds);

	// then fetch the feeds of all categories
	for (int i=0; i<catsize; i++) {
		json_object* cat = (json_object*)array_list_get_idx(categories, i);
		fetch_feeds_per_category(cat, feeds);
	}

	json_object_put(content);

	return feeds;
}

void ttrss_api::add_custom_headers(curl_slist** /* custom_headers */) {
	// nothing required
}

bool ttrss_api::mark_all_read(const std::string& feed_url) {

	std::map<std::string, std::string> args;
	args["feed_id"] = url_to_id(feed_url);
	json_object* content = run_op("catchupFeed", args);

	if (!content)
		return false;

	json_object_put(content);
	return true;
}

bool ttrss_api::mark_article_read(const std::string& guid, bool read) {

	// Do this in a thread, as we don't care about the result enough to wait for
	// it.
	std::thread t {markreadthread(this, guid, read)};
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
	json_object* content = run_op("getHeadlines", args);

	if (!content)
		return f;

	if (json_object_get_type(content) != json_type_array) {
		LOG(level::ERROR, "ttrss_api::fetch_feed: content is not an array");
		return f;
	}

	struct array_list * items = json_object_get_array(content);
	int items_size = array_list_length(items);
	LOG(level::DEBUG, "ttrss_api::fetch_feed: %d items", items_size);

	for (int i=0; i<items_size; i++) {
		json_object* item_obj = (json_object*)array_list_get_idx(items, i);

		rsspp::item item;

		json_object* node {};

		if (json_object_object_get_ex(item_obj, "title", &node) == TRUE) {
			item.title = json_object_get_string(node);
		}

		if (json_object_object_get_ex(item_obj, "link", &node) == TRUE) {
			item.link = json_object_get_string(node);
		}

		if (json_object_object_get_ex(item_obj, "author", &node) == TRUE) {
			item.author = json_object_get_string(node);
		}

		if (json_object_object_get_ex(item_obj, "content", &node) == TRUE) {
			item.content_encoded = json_object_get_string(node);
		}

		json_object * attachments {};
		if (json_object_object_get_ex(item_obj, "attachments", &attachments)
				== TRUE)
		{
			struct array_list * attachments_list = json_object_get_array(attachments);
			int attachments_size = array_list_length(attachments_list);
			if (attachments_size > 0) {
				json_object* attachment =
				    (json_object*)array_list_get_idx(attachments_list, 0);

				if (json_object_object_get_ex(attachment, "content_url", &node)
						== TRUE)
				{
					item.enclosure_url = json_object_get_string(node);
				}

				if (json_object_object_get_ex(attachment, "content_type", &node)
						== TRUE)
				{
					item.enclosure_type = json_object_get_string(node);
				}
			}
		}

		json_object_object_get_ex(item_obj, "id", &node);
		int id = json_object_get_int(node);
		item.guid = strprintf::fmt("%d", id);

		json_object_object_get_ex(item_obj, "unread", &node);
		json_bool unread = json_object_get_boolean(node);
		if (unread) {
			item.labels.push_back("ttrss:unread");
		} else {
			item.labels.push_back("ttrss:read");
		}

		json_object_object_get_ex(item_obj, "updated", &node);
		time_t updated = (time_t)json_object_get_int(node);
		char rfc822_date[128];
		strftime(rfc822_date, sizeof(rfc822_date), "%a, %d %b %Y %H:%M:%S %z", gmtime(&updated));
		item.pubDate = rfc822_date;
		item.pubDate_ts = updated;

		f.items.push_back(item);
	}

	std::sort(f.items.begin(), f.items.end(), [](const rsspp::item& a, const rsspp::item& b) {
		return a.pubDate_ts > b.pubDate_ts;
	});

	json_object_put(content);
	return f;
}

void ttrss_api::fetch_feeds_per_category(
    json_object * cat, std::vector<tagged_feedurl>& feeds)
{
	const char * cat_name = nullptr;
	json_object * cat_title_obj {};
	int cat_id;

	if (cat) {
		json_object* cat_id_obj {};
		json_object_object_get_ex(cat, "id", &cat_id_obj);
		cat_id = json_object_get_int(cat_id_obj);

		// ignore special categories, for now
		if (cat_id < 0)
			return;

		json_object_object_get_ex(cat, "title", &cat_title_obj);
		cat_name = json_object_get_string(cat_title_obj);
		LOG(level::DEBUG, "ttrss_api::fetch_feeds_per_category: id = %d title = %s", cat_id, cat_name);
	} else {
		// As uncategorized is a category itself (id = 0) and the default value
		// for a getFeeds is id = 0, the feeds in uncategorized will appear twice
		return;
	}

	std::map<std::string, std::string> args;
	if (cat)
		args["cat_id"] = std::to_string(cat_id);
	json_object* feed_list_obj = run_op("getFeeds", args);

	if (!feed_list_obj)
		return;

	struct array_list * feed_list = json_object_get_array(feed_list_obj);

	int feed_list_size = array_list_length(feed_list);

	for (int j=0; j<feed_list_size; j++) {
		json_object* feed = (json_object*)array_list_get_idx(feed_list, j);

		json_object* node {};

		json_object_object_get_ex(feed, "id", &node);
		int feed_id = json_object_get_int(node);

		json_object_object_get_ex(feed, "title", &node);
		const char * feed_title = json_object_get_string(node);

		json_object_object_get_ex(feed, "feed_url", &node);
		const char * feed_url = json_object_get_string(node);

		std::vector<std::string> tags;
		tags.push_back(std::string("~") + feed_title);

		if (cat_name) {
			tags.push_back(cat_name);
		}

		auto url = strprintf::fmt("%s#%d", feed_url, feed_id);
		feeds.push_back(tagged_feedurl(url, tags));

		// TODO: cache feed_id -> feed_url (or feed_url -> feed_id ?)
	}

	json_object_put(feed_list_obj);

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
	json_object* content = run_op("updateArticle", args);

	if (!content)
		return false;

	json_object_put(content);
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

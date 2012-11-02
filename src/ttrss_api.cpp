#include <json.h>
#include <remote_api.h>
#include <ttrss_api.h>
#include <cstring>
#include <algorithm>

#include <markreadthread.h>

namespace newsbeuter {

ttrss_api::ttrss_api(configcontainer * c) : remote_api(c) {
	single = (cfg->get_configvalue("ttrss-mode") == "single");
	auth_info = utils::strprintf("%s:%s", cfg->get_configvalue("ttrss-login").c_str(), cfg->get_configvalue("ttrss-password").c_str());
	auth_info_ptr = auth_info.c_str();
	sid = "";
}

ttrss_api::~ttrss_api() {
}

bool ttrss_api::authenticate() {
	 if (auth_lock.trylock()) {
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
	CURL * handle = curl_easy_init();
	char * user = curl_easy_escape(handle, cfg->get_configvalue("ttrss-login").c_str(), 0);
	char * pass = curl_easy_escape(handle, cfg->get_configvalue("ttrss-password").c_str(), 0);

	std::map<std::string, std::string> args;
	args["user"] = single ? "admin" : std::string(user);
	args["password"] = std::string(pass);
	struct json_object * content = run_op("login", args);

	curl_free(user);
	curl_free(pass);

	if (content == NULL)
		return "";

	std::string sid;

	struct json_object * session_id = json_object_object_get(content, "session_id");
	sid = json_object_get_string(session_id);

	json_object_put(content);

	LOG(LOG_DEBUG, "ttrss_api::retrieve_sid: sid = '%s'", sid.c_str());

	return sid;
}

struct json_object * ttrss_api::run_op(const std::string& op,
				       const std::map<std::string, std::string >& args,
				       bool try_login) {
	std::string url = utils::strprintf("%s/api/?op=%s&sid=%s", cfg->get_configvalue("ttrss-url").c_str(),
		op.c_str(), sid.c_str());

	for (std::map<std::string, std::string>::const_iterator it = args.begin(); it != args.end(); it++) {
		url += "&" + it->first + "=" + it->second;
	}
	std::string result = utils::retrieve_url(url, cfg, auth_info_ptr);

	LOG(LOG_DEBUG, "ttrss_api::run_op(%s,...): reply = %s", op.c_str(), result.c_str());

	struct json_object * reply = json_tokener_parse(result.c_str());
	if (is_error(reply)) {
		LOG(LOG_ERROR, "ttrss_api::run_op: reply failed to parse: %s", result.c_str());
		return NULL;
	}

	struct json_object * status = json_object_object_get(reply, "status");
	if (is_error(status)) {
		LOG(LOG_ERROR, "ttrss_api::run_op: no status code");
		return NULL;
	}

	struct json_object * content = json_object_object_get(reply, "content");
	if (is_error(content)) {
		LOG(LOG_ERROR, "ttrss_api::run_op: no content part in answer from server");
		return NULL;
	}
	
	if (json_object_get_int(status) != 0) {
		struct json_object * error = json_object_object_get(content, "error");
		if ((strcmp(json_object_get_string(error), "NOT_LOGGED_IN") == 0) && try_login) {
			json_object_put(reply);
			if (authenticate())
				return run_op(op, args, false);
			else
				return NULL;
		} else {
			json_object_put(reply);
			return NULL;
		}
	}

	// free the parent object, without freeing content as well:
	json_object_get(content);
	json_object_put(reply);
	return content;
}

std::vector<tagged_feedurl> ttrss_api::get_subscribed_urls() {

	std::string cat_url = utils::strprintf("%s/api/?op=getCategories&sid=%s", cfg->get_configvalue("ttrss-url").c_str(), sid.c_str());
	std::string result = utils::retrieve_url(cat_url, cfg, auth_info_ptr);

	LOG(LOG_DEBUG, "ttrss_api::get_subscribed_urls: reply = %s", result.c_str());

	std::vector<tagged_feedurl> feeds;

	struct json_object * content = run_op("getCategories", std::map<std::string, std::string>());
	if (!content)
		return feeds;

	if (json_object_get_type(content) != json_type_array)
		return feeds;

	struct array_list * categories = json_object_get_array(content);

	int catsize = array_list_length(categories);

	// first fetch feeds within no category
	fetch_feeds_per_category(NULL, feeds);

	// then fetch the feeds of all categories
	for (int i=0;i<catsize;i++) {
		struct json_object * cat = (struct json_object *)array_list_get_idx(categories, i);
		fetch_feeds_per_category(cat, feeds);
	}

	json_object_put(content);

	return feeds;
}

void ttrss_api::configure_handle(CURL * /*handle*/) {
	// nothing required
}

bool ttrss_api::mark_all_read(const std::string& feed_url) {

	std::map<std::string, std::string> args;
	args["feed_id"] = url_to_id(feed_url);
	struct json_object * content = run_op("catchupFeed", args);

	if(!content)
		return false;

	json_object_put(content);
	return true;
}

bool ttrss_api::mark_article_read(const std::string& guid, bool read) {

	// Do this in a thread, as we don't care about the result enough to wait for
	// it.
	thread * mrt = new markreadthread( this, guid, read );
	mrt->start();
	return true;

}

bool ttrss_api::update_article_flags(const std::string& oldflags, const std::string& newflags, const std::string& guid) {
	std::string star_flag = cfg->get_configvalue("ttrss-flag-star");
	std::string publish_flag = cfg->get_configvalue("ttrss-flag-publish");
	bool success = true;

	if (star_flag.length() > 0) {
		if (strchr(oldflags.c_str(), star_flag[0])==NULL && strchr(newflags.c_str(), star_flag[0])!=NULL) {
			success = star_article(guid, true);
		} else if (strchr(oldflags.c_str(), star_flag[0])!=NULL && strchr(newflags.c_str(), star_flag[0])==NULL) {
			success = star_article(guid, false);
		}
	}

	if (publish_flag.length() > 0) {
		if (strchr(oldflags.c_str(), publish_flag[0])==NULL && strchr(newflags.c_str(), publish_flag[0])!=NULL) {
			success = publish_article(guid, true);
		} else if (strchr(oldflags.c_str(), publish_flag[0])!=NULL && strchr(newflags.c_str(), publish_flag[0])==NULL) {
			success = publish_article(guid, false);
		}
	}

	return success;
}

static bool sort_by_pubdate(const rsspp::item& a, const rsspp::item& b) {
	return a.pubDate_ts > b.pubDate_ts;
}

rsspp::feed ttrss_api::fetch_feed(const std::string& id) {
	rsspp::feed f;

	f.rss_version = rsspp::TTRSS_JSON;

	std::map<std::string, std::string> args;
	args["feed_id"] = id;
	args["show_content"] = "1";
	struct json_object * content = run_op("getHeadlines", args);

	if (!content)
		return f;

	if (json_object_get_type(content) != json_type_array) {
		LOG(LOG_ERROR, "ttrss_api::fetch_feed: content is not an array");
		return f;
	}

	struct array_list * items = json_object_get_array(content);
	int items_size = array_list_length(items);
	LOG(LOG_DEBUG, "ttrss_api::fetch_feed: %d items", items_size);

	for (int i=0;i<items_size;i++) {
		struct json_object * item_obj = (struct json_object *)array_list_get_idx(items, i);
		int id = json_object_get_int(json_object_object_get(item_obj, "id"));
		const char * title = json_object_get_string(json_object_object_get(item_obj, "title"));
		const char * link = json_object_get_string(json_object_object_get(item_obj, "link"));
		const char * content = json_object_get_string(json_object_object_get(item_obj, "content"));
		time_t updated = (time_t)json_object_get_int(json_object_object_get(item_obj, "updated"));
		bool unread = json_object_get_boolean(json_object_object_get(item_obj, "unread"));

		rsspp::item item;

		if (title)
			item.title = title;

		if (link)
			item.link = link;

		if (content)
			item.content_encoded = content;

		item.guid = utils::strprintf("%d", id);

		if (unread) {
			item.labels.push_back("ttrss:unread");
		} else {
			item.labels.push_back("ttrss:read");
		}

		char rfc822_date[128];
		strftime(rfc822_date, sizeof(rfc822_date), "%a, %d %b %Y %H:%M:%S %z", gmtime(&updated));
		item.pubDate = rfc822_date;
		item.pubDate_ts = updated;

		f.items.push_back(item);
	}

	std::sort(f.items.begin(), f.items.end(), sort_by_pubdate);

	json_object_put(content);
	return f;
}

void ttrss_api::fetch_feeds_per_category(struct json_object * cat, std::vector<tagged_feedurl>& feeds) {
	const char * cat_name = NULL;
	struct json_object * cat_title_obj = NULL;
	int cat_id;

	if (cat) {
		struct json_object * cat_id_obj = json_object_object_get(cat, "id");
		cat_id = json_object_get_int(cat_id_obj);

		cat_title_obj = json_object_object_get(cat, "title");
		cat_name = json_object_get_string(cat_title_obj);
		LOG(LOG_DEBUG, "ttrss_api::fetch_feeds_per_category: id = %d title = %s", cat_id, cat_name);
	}

	std::map<std::string, std::string> args;
	if (cat)
		args["cat_id"] = utils::to_s(cat_id);
	struct json_object * feed_list_obj = run_op("getFeeds", args);

	if (!feed_list_obj)
		return;

	struct array_list * feed_list = json_object_get_array(feed_list_obj);

	int feed_list_size = array_list_length(feed_list);

	for (int j=0;j<feed_list_size;j++) {
		struct json_object * feed = (struct json_object *)array_list_get_idx(feed_list, j);

		int feed_id = json_object_get_int(json_object_object_get(feed, "id"));
		const char * feed_title = json_object_get_string(json_object_object_get(feed, "title"));
		const char * feed_url = json_object_get_string(json_object_object_get(feed, "feed_url"));

		std::vector<std::string> tags;
		tags.push_back(std::string("~") + feed_title);
		if (cat_name) {
			tags.push_back(cat_name);
		}
		feeds.push_back(tagged_feedurl(utils::strprintf("%s#%d", feed_url, feed_id), tags));

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
	args["field"] = utils::to_s(field);
	args["mode"] = utils::to_s(mode);
	struct json_object * content = run_op("updateArticle", args);

	if (!content)
	    return false;

	json_object_put(content);
	return true;
}

std::string ttrss_api::url_to_id(const std::string& url) {
	const char * uri = url.c_str();
	const char * pound = strrchr(uri, '#');
	if (!pound)
		return "";
	return std::string(pound+1);
}


}

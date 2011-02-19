#include <json.h>
#include <remote_api.h>
#include <ttrss_api.h>
#include <cstring>

namespace newsbeuter {

ttrss_api::ttrss_api(configcontainer * c) : remote_api(c) {
	single = (cfg->get_configvalue("ttrss-mode") == "single");
	auth_info = utils::strprintf("%s:%s", cfg->get_configvalue("ttrss-login").c_str(), cfg->get_configvalue("ttrss-password").c_str());
	auth_info_ptr = auth_info.c_str();
}

ttrss_api::~ttrss_api() {
}

bool ttrss_api::authenticate() {
	sid = retrieve_sid();

	return sid != "";
}

std::string ttrss_api::retrieve_sid() {
	CURL * handle = curl_easy_init();
	const std::string location = cfg->get_configvalue("ttrss-url");
	char * user = curl_easy_escape(handle, cfg->get_configvalue("ttrss-login").c_str(), 0);
	char * pass = curl_easy_escape(handle, cfg->get_configvalue("ttrss-password").c_str(), 0);

	std::string login_url; 
	if (single) {
		login_url = utils::strprintf("%s/api/?op=login&user=admin&password=", location.c_str());
	} else {
		login_url = utils::strprintf("%s/api/?op=login&user=%s&password=%s", location.c_str(), user, pass);
	}

	curl_free(user);
	curl_free(pass);

	std::string result = utils::retrieve_url(login_url, cfg, auth_info_ptr);

	std::string sid;

	struct json_object * reply = json_tokener_parse(result.c_str());
	if (is_error(reply)) {
		return "";
	}

	struct json_object * status = json_object_object_get(reply, "status");
	if (json_object_get_int(status) == 0) {
		struct json_object * content = json_object_object_get(reply, "content");
		struct json_object * session_id = json_object_object_get(content, "session_id");
		sid = json_object_get_string(session_id);
	}

	json_object_put(reply);

	LOG(LOG_DEBUG, "ttrss_api::retrieve_sid: result = '%s' sid = '%s'", result.c_str(), sid.c_str());

	return sid;
}

std::vector<tagged_feedurl> ttrss_api::get_subscribed_urls() {
	std::string cat_url = utils::strprintf("%s/api/?op=getCategories&sid=%s", cfg->get_configvalue("ttrss-url").c_str(), sid.c_str());
	std::string result = utils::retrieve_url(cat_url, cfg, auth_info_ptr);

	LOG(LOG_DEBUG, "ttrss_api::get_subscribed_urls: reply = %s", result.c_str());

	std::vector<tagged_feedurl> feeds;

	struct json_object * reply = json_tokener_parse(result.c_str());
	if (is_error(reply)) {
		return feeds;
	}

	struct json_object * status = json_object_object_get(reply, "status");
	if (json_object_get_int(status) != 0)
		return feeds;

	struct json_object * content = json_object_object_get(reply, "content");

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

	json_object_put(reply);

	return feeds;
}

void ttrss_api::configure_handle(CURL * /*handle*/) {
	// nothing required
}

bool ttrss_api::mark_all_read(const std::string& feed_url) {
	std::string catchup_url = utils::strprintf("%s/api/?op=catchupFeed&feed_id=%s&sid=%s", 
			cfg->get_configvalue("ttrss-url").c_str(), feed_url.substr(6,feed_url.length()-6).c_str(), sid.c_str());

	std::string result = utils::retrieve_url(catchup_url, cfg, auth_info_ptr);

	LOG(LOG_DEBUG, "ttrss_api::mark_all_read: result = %s", result.c_str());

	struct json_object * reply = json_tokener_parse(result.c_str());

	if (is_error(reply)) {
		return false;
	}

	struct json_object * status = json_object_object_get(reply, "status");
	if (json_object_get_int(status) != 0) {
		json_object_put(reply);
		return false;
	}

	struct json_object * content = json_object_object_get(reply, "content");

	if (strcmp(json_object_get_string(json_object_object_get(content, "status")), "OK") != 0) {
		json_object_put(reply);
		return false;
	}

	json_object_put(reply);
	return true;
}

bool ttrss_api::mark_article_read(const std::string& guid, bool read) {
	return update_article(guid, 2, read ? 0 : 1);
}

bool ttrss_api::update_article_flags(const std::string& oldflags, const std::string& newflags, const std::string& guid) {
	std::string star_flag = cfg->get_configvalue("ttrss-flag-star");
	std::string publish_flag = cfg->get_configvalue("ttrss-flag-publish");
	bool success;

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

rsspp::feed ttrss_api::fetch_feed(const std::string& id) {
	rsspp::feed f;

	f.rss_version = rsspp::TTRSS_JSON;

	std::string feed_url = utils::strprintf("%s/api/?op=getHeadlines&feed_id=%s&show_content=1&sid=%s", 
		cfg->get_configvalue("ttrss-url").c_str(), id.c_str(), sid.c_str());
	std::string result = utils::retrieve_url(feed_url, cfg, auth_info_ptr);

	LOG(LOG_DEBUG, "ttrss_api::fetch_feed: %s", result.c_str());

	struct json_object * reply = json_tokener_parse(result.c_str());
	if (is_error(reply)) {
		return f;
	}

	int rc;
	if ((rc=json_object_get_int(json_object_object_get(reply, "status"))) != 0) {
		LOG(LOG_ERROR, "ttrss_api::fetch_feed: status = %d", rc);
		return f;
	}

	struct json_object * content = json_object_object_get(reply, "content");
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
		boolean unread = json_object_get_boolean(json_object_object_get(item_obj, "unread"));

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

		f.items.push_back(item);
	}

	json_object_put(reply);
	return f;
}

void ttrss_api::fetch_feeds_per_category(struct json_object * cat, std::vector<tagged_feedurl>& feeds) {
	const char * cat_name = NULL;
	struct json_object * cat_title_obj = NULL;
	int cat_id;
	std::string feeds_url;
	
	if (cat) {
		struct json_object * cat_id_obj = json_object_object_get(cat, "id");
		cat_id = json_object_get_int(cat_id_obj);

		cat_title_obj = json_object_object_get(cat, "title");
		cat_name = json_object_get_string(cat_title_obj);
		LOG(LOG_DEBUG, "ttrss_api::fetch_feeds_per_category: id = %d title = %s", cat_id, cat_name);

		feeds_url = utils::strprintf("%s/api/?op=getFeeds&cat_id=%d&sid=%s", cfg->get_configvalue("ttrss-url").c_str(), cat_id, sid.c_str());
	} else {
		feeds_url = utils::strprintf("%s/api/?op=getFeeds&sid=%s", cfg->get_configvalue("ttrss-url").c_str(), sid.c_str());
	}

	std::string feeds_data = utils::retrieve_url(feeds_url, cfg, auth_info_ptr);

	LOG(LOG_DEBUG, "ttrss_api::fetch_feeds_per_category: feeds_data = %s", feeds_data.c_str());

	struct json_object * feeds_reply = json_tokener_parse(feeds_data.c_str());
	if (is_error(feeds_reply)) {
		return;
	}

	struct json_object * feed_list_obj = json_object_object_get(feeds_reply, "content");
	struct array_list * feed_list = json_object_get_array(feed_list_obj);

	int feed_list_size = array_list_length(feed_list);

	for (int j=0;j<feed_list_size;j++) {
		struct json_object * feed = (struct json_object *)array_list_get_idx(feed_list, j);

		int feed_id = json_object_get_int(json_object_object_get(feed, "id"));
		const char * feed_title = json_object_get_string(json_object_object_get(feed, "title"));

		std::vector<std::string> tags;
		tags.push_back(std::string("~") + feed_title);
		if (cat_name) {
			tags.push_back(cat_name);
		}
		feeds.push_back(tagged_feedurl(utils::strprintf("ttrss:%d", feed_id), tags));

		// TODO: cache feed_id -> feed_url (or feed_url -> feed_id ?)
	}

	json_object_put(feeds_reply);

}

bool ttrss_api::star_article(const std::string& guid, bool star) {
	return update_article(guid, 0, star ? 1 : 0);
}

bool ttrss_api::publish_article(const std::string& guid, bool publish) {
	return update_article(guid, 1, publish ? 1 : 0);
}

bool ttrss_api::update_article(const std::string& guid, int field, int mode) {
	std::string update_url = utils::strprintf("%s/api/?op=updateArticle&article_ids=%s&field=%d&mode=%d&sid=%s", 
			cfg->get_configvalue("ttrss-url").c_str(), guid.c_str(), field, mode, sid.c_str());

	std::string result = utils::retrieve_url(update_url, cfg, auth_info_ptr);

	LOG(LOG_DEBUG, "ttrss_api::update_article: result = %s", result.c_str());

	struct json_object * reply = json_tokener_parse(result.c_str());
	if (is_error(reply)) {
		return false;
	}

	struct json_object * status = json_object_object_get(reply, "status");
	if (json_object_get_int(status) != 0) {
		json_object_put(reply);
		return false;
	}

	struct json_object * content = json_object_object_get(reply, "content");

	if (strcmp(json_object_get_string(json_object_object_get(content, "status")), "OK") != 0) {
		json_object_put(reply);
		return false;
	}

	json_object_put(reply);
	return true;
}


}

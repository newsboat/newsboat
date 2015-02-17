#include <rsspp.h>
#include <json.h>
#include <utils.h>
#include <remote_api.h>
#include <newsblur_api.h>
#include <algorithm>
#include <string.h>

#define NEWSBLUR_ITEMS_PER_PAGE 6

namespace newsbeuter {

newsblur_api::newsblur_api(configcontainer * c) : remote_api(c) {
	auth_info = utils::strprintf("username=%s&password=%s", cfg->get_configvalue("newsblur-login").c_str(), cfg->get_configvalue("newsblur-password").c_str());
	api_location = cfg->get_configvalue("newsblur-url");
	min_pages = (cfg->get_configvalue_as_int("newsblur-min-items") + (NEWSBLUR_ITEMS_PER_PAGE + 1)) / NEWSBLUR_ITEMS_PER_PAGE;

	if(cfg->get_configvalue("cookie-cache").empty()) {
		LOG(LOG_CRITICAL, "newsblur_api::newsblur_api: No cookie-cache has been configured the login won't work.");
	}
}

newsblur_api::~newsblur_api() {
}

bool newsblur_api::authenticate() {
	json_object * response;
	json_object * status;

	response = newsblur_api::query_api("/api/login", &auth_info);
	status = json_object_object_get(response, "authenticated");
	bool result = json_object_get_boolean(status);
	LOG(LOG_INFO, "newsblur_api::authenticate: authentication resulted in %u, cached in %s", result, cfg->get_configvalue("cookie-cache").c_str());
	return result;
}

std::vector<tagged_feedurl> newsblur_api::get_subscribed_urls() {
	std::vector<tagged_feedurl> result;

	json_object * response = query_api("/reader/feeds", NULL);

	json_object * feeds = json_object_object_get(response, "feeds");

	json_object_iterator it = json_object_iter_begin(feeds);
	json_object_iterator itEnd = json_object_iter_end(feeds);

	while (!json_object_iter_equal(&it, &itEnd)) {
		const char * feed_id = json_object_iter_peek_name(&it);
		json_object * node;
		rsspp::feed current_feed;

		current_feed.rss_version = rsspp::NEWSBLUR_JSON;

		json_object * feed_json = json_object_iter_peek_value(&it);
		node = json_object_object_get(feed_json, "feed_title");
		current_feed.title = json_object_get_string(node);
		node = json_object_object_get(feed_json, "feed_link");
		current_feed.link = json_object_get_string(node);

		known_feeds[feed_id] = current_feed;

		std::vector<std::string> tags = std::vector<std::string>();
		result.push_back(tagged_feedurl(std::string(feed_id), tags));

		json_object_iter_next(&it);
	}

	return result;
}

void newsblur_api::configure_handle(CURL * /*handle*/) {
	// nothing required
}

bool request_successfull(json_object * payload) {
	json_object * result = json_object_object_get(payload, "result");
	if (result == NULL)
		return false;

	return !strcmp("ok", json_object_get_string(result));
}

bool newsblur_api::mark_all_read(const std::string& feed_url) {
	std::string post_data = utils::strprintf("feed_id=%s", feed_url.c_str());
	json_object * query_result = query_api("/reader/mark_feed_as_read", &post_data);
	return request_successfull(query_result);
}

bool newsblur_api::mark_article_read(const std::string& guid, bool read) {
	std::string endpoint;
	int separator = guid.find(ID_SEPARATOR);
	std::string feed_id = guid.substr(0, separator);
	std::string article_id = guid.substr(separator + sizeof(ID_SEPARATOR) - 1);

	std::string post_data = "feed_id=" + feed_id + "&" + "story_id=" + article_id;
	if(read) {
		endpoint = "/reader/mark_story_as_read";
	} else {
		endpoint = "/reader/mark_story_as_unread";
	}

	json_object * query_result = query_api(endpoint, &post_data);
	return request_successfull(query_result);
}

bool newsblur_api::update_article_flags(const std::string& oldflags, const std::string& newflags, const std::string& guid) {
	(void)oldflags;
	(void)newflags;
	(void)guid;
	return false;
}

time_t parse_date(const char * raw) {
	struct tm tm;
	memset(&tm, 0, sizeof(tm));

	strptime(raw, "%Y-%m-%d %H:%M:%S", &tm);
	return mktime(&tm);
}

rsspp::feed newsblur_api::fetch_feed(const std::string& id) {
	rsspp::feed f = known_feeds[id];

	LOG(LOG_INFO, "newsblur_api::fetch_feed: about to fetch %u pages of feed %s", min_pages, id.c_str());

	for(unsigned int i = 1; i <= min_pages; i++) {

		std::string page = utils::to_string(i);

		json_object * query_result = query_api("/reader/feed/" + id + "?page=" + page, NULL);

		if (!query_result)
			return f;

		json_object * stories = json_object_object_get(query_result, "stories");

		if (!stories) {
			LOG(LOG_ERROR, "newsblur_api::fetch_feed: request returned no stories");
			return f;
		}

		if (json_object_get_type(stories) != json_type_array) {
			LOG(LOG_ERROR, "newsblur_api::fetch_feed: content is not an array");
			return f;
		}

		struct array_list * items = json_object_get_array(stories);
		int items_size = array_list_length(items);
		LOG(LOG_DEBUG, "newsblur_api::fetch_feed: %d items", items_size);

		for (int i = 0; i < items_size; i++) {
			struct json_object * item_obj = (struct json_object *)array_list_get_idx(items, i);
			const char * article_id = json_object_get_string(json_object_object_get(item_obj, "id"));
			const char * title = json_object_get_string(json_object_object_get(item_obj, "story_title"));
			const char * link = json_object_get_string(json_object_object_get(item_obj, "story_permalink"));
			const char * content = json_object_get_string(json_object_object_get(item_obj, "story_content"));
			const char * pub_date = json_object_get_string(json_object_object_get(item_obj, "story_date"));
			bool read_status = json_object_get_int(json_object_object_get(item_obj, "read_status"));

			rsspp::item item;

			if (title)
				item.title = title;

			if (link)
				item.link = link;

			if (content)
				item.content_encoded = content;

			item.guid = id + ID_SEPARATOR + article_id;

			if (read_status == 0) {
				item.labels.push_back("newsblur:unread");
			} else if (read_status == 1) {
				item.labels.push_back("newsblur:read");
			}

			item.pubDate_ts = parse_date(pub_date);
			char rfc822_date[128];
			strftime(rfc822_date, sizeof(rfc822_date), "%a, %d %b %Y %H:%M:%S %z", gmtime(&item.pubDate_ts));
			item.pubDate = rfc822_date;

			f.items.push_back(item);
		}
	}

	std::sort(f.items.begin(), f.items.end(), [](const rsspp::item& a, const rsspp::item& b) {
		return a.pubDate_ts > b.pubDate_ts;
	});
	return f;
}

json_object * newsblur_api::query_api(const std::string& endpoint, const std::string* postdata) {

	const char * url = (api_location + endpoint).c_str();
	std::string data = utils::retrieve_url(url, cfg, NULL, postdata);

	json_object * result =  json_tokener_parse(data.c_str());
	if(!result)
		LOG(LOG_WARN, "newsblur_api::query_api: request to %s failed", url);
	return result;
}

}

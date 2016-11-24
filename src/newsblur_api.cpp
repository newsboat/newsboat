#include <rsspp.h>
#include <json.h>
#include <utils.h>
#include <strprintf.h>
#include <remote_api.h>
#include <newsblur_api.h>
#include <algorithm>
#include <string.h>
#include <time.h>

#define NEWSBLUR_ITEMS_PER_PAGE 6

namespace newsbeuter {

newsblur_api::newsblur_api(configcontainer * c) : remote_api(c) {
	auth_info = strprintf::fmt("username=%s&password=%s", cfg->get_configvalue("newsblur-login"), cfg->get_configvalue("newsblur-password"));
	api_location = cfg->get_configvalue("newsblur-url");
	min_pages = (cfg->get_configvalue_as_int("newsblur-min-items") + (NEWSBLUR_ITEMS_PER_PAGE + 1)) / NEWSBLUR_ITEMS_PER_PAGE;

	if (cfg->get_configvalue("cookie-cache").empty()) {
		LOG(level::CRITICAL, "newsblur_api::newsblur_api: No cookie-cache has been configured the login won't work.");
	}
}

newsblur_api::~newsblur_api() {
}

bool newsblur_api::authenticate() {
	json_object * response {};
	json_object * status {};

	response = newsblur_api::query_api("/api/login", &auth_info);
	json_object_object_get_ex(response, "authenticated", &status);
	bool result = json_object_get_boolean(status);

	LOG(
	    level::INFO,
	    "newsblur_api::authenticate: authentication resulted in %u, cached in %s",
	    result,
	    cfg->get_configvalue("cookie-cache"));

	return result;
}

std::vector<tagged_feedurl> newsblur_api::get_subscribed_urls() {
	std::vector<tagged_feedurl> result;

	json_object * response = query_api("/reader/feeds/", nullptr);

	json_object * feeds {};
	json_object_object_get_ex(response, "feeds", &feeds);

	json_object_iterator it = json_object_iter_begin(feeds);
	json_object_iterator itEnd = json_object_iter_end(feeds);

	json_object * folders {};
	json_object_object_get_ex(response, "folders", &folders);

	std::map<std::string, std::vector<std::string>> feeds_to_tags = mk_feeds_to_tags(folders);

	while (!json_object_iter_equal(&it, &itEnd)) {
		const char * feed_id = json_object_iter_peek_name(&it);
		json_object * node {};
		rsspp::feed current_feed;

		current_feed.rss_version = rsspp::NEWSBLUR_JSON;

		json_object * feed_json = json_object_iter_peek_value(&it);
		json_object_object_get_ex(feed_json, "feed_title", &node);
		current_feed.title = json_object_get_string(node);
		json_object_object_get_ex(feed_json, "feed_link", &node);
		current_feed.link = json_object_get_string(node);

		known_feeds[feed_id] = current_feed;

		std::string std_feed_id(feed_id);
		std::vector<std::string> tags = feeds_to_tags[std_feed_id];
		result.push_back(tagged_feedurl(std_feed_id, tags));

		json_object_iter_next(&it);
	}

	return result;
}

std::map<std::string, std::vector<std::string>> newsblur_api::mk_feeds_to_tags(
json_object * folders)
{
	std::map<std::string, std::vector<std::string>> result;
	array_list * tags = json_object_get_array(folders);
	int tags_len = array_list_length(tags);
	for (int i = 0; i < tags_len; ++i) {
		json_object * tag_to_feed_ids = json_object_array_get_idx(folders, i);

		if (!json_object_is_type(tag_to_feed_ids, json_type_object))
			// "folders" array contains not only dictionaries describing
			// folders but also numbers, which are IDs of feeds that don't
			// belong to any folder. This check skips these IDs.
			continue;

		json_object_object_foreach(tag_to_feed_ids, key, feeds_with_tag_obj) {
			std::string std_key(key);
			array_list * feeds_with_tag_arr = json_object_get_array(feeds_with_tag_obj);
			int feeds_with_tag_len = array_list_length(feeds_with_tag_arr);
			for (int j = 0; j < feeds_with_tag_len; ++j) {
				json_object * feed_id_obj = json_object_array_get_idx(feeds_with_tag_obj, j);
				std::string feed_id(json_object_get_string(feed_id_obj));
				result[feed_id].push_back(std_key);
			}
		}
	}
	return result;
}

void newsblur_api::add_custom_headers(curl_slist** /* custom_headers */) {
	// nothing required
}

bool request_successfull(json_object * payload) {
	json_object * result {};
	if (json_object_object_get_ex(payload, "result", &result) == FALSE) {
		return false;
	} else {
		return !strcmp("ok", json_object_get_string(result));
	}
}

bool newsblur_api::mark_all_read(const std::string& feed_url) {
	std::string post_data = strprintf::fmt("feed_id=%s", feed_url);
	json_object * query_result = query_api("/reader/mark_feed_as_read", &post_data);
	return request_successfull(query_result);
}

bool newsblur_api::mark_article_read(const std::string& guid, bool read) {
	// handle dummy articles
	if (guid.empty()) {
		return false;
	}
	std::string endpoint;
	int separator = guid.find(ID_SEPARATOR);
	std::string feed_id = guid.substr(0, separator);
	std::string article_id = guid.substr(separator + sizeof(ID_SEPARATOR) - 1);

	std::string post_data = "feed_id=" + feed_id + "&" + "story_id=" + article_id;
	if (read) {
		endpoint = "/reader/mark_story_as_read";
	} else {
		endpoint = "/reader/mark_story_as_unread";
	}

	json_object * query_result = query_api(endpoint, &post_data);
	return request_successfull(query_result);
}

bool newsblur_api::update_article_flags(
		const std::string& /* oldflags */,
		const std::string& /* newflags */,
		const std::string& /* guid */)
{
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

	LOG(level::INFO, "newsblur_api::fetch_feed: about to fetch %u pages of feed %s", min_pages, id);

	for (unsigned int i = 1; i <= min_pages; i++) {

		std::string page = std::to_string(i);

		json_object * query_result = query_api("/reader/feed/" + id + "?page=" + page, nullptr);

		if (!query_result)
			return f;

		json_object * stories {};
		if (json_object_object_get_ex(query_result, "stories", &stories)
				== FALSE)
		{
			LOG(level::ERROR, "newsblur_api::fetch_feed: request returned no stories");
			return f;
		}

		if (json_object_get_type(stories) != json_type_array) {
			LOG(level::ERROR, "newsblur_api::fetch_feed: content is not an array");
			return f;
		}

		struct array_list * items = json_object_get_array(stories);
		int items_size = array_list_length(items);
		LOG(level::DEBUG, "newsblur_api::fetch_feed: %d items", items_size);

		for (int i = 0; i < items_size; i++) {
			json_object* item_obj = (json_object*)array_list_get_idx(items, i);

			rsspp::item item;

			json_object* node {};

			if (json_object_object_get_ex(item_obj, "story_title", &node)
					== TRUE)
			{
				item.title = json_object_get_string(node);
			}

			if (json_object_object_get_ex(item_obj, "story_permalink", &node)
					== TRUE)
			{
				item.link = json_object_get_string(node);
			}

			if (json_object_object_get_ex(item_obj, "story_content", &node)
					== TRUE)
			{
				item.content_encoded = json_object_get_string(node);
			}

			const char * article_id {};
			if (json_object_object_get_ex(item_obj, "id", &node) == TRUE) {
				article_id = json_object_get_string(node);
			}
			item.guid = id + ID_SEPARATOR + article_id;

			if (json_object_object_get_ex(item_obj, "read_status", &node)
					== TRUE)
			{
				if (! static_cast<bool>(json_object_get_int(node))) {
					item.labels.push_back("newsblur:unread");
				} else {
					item.labels.push_back("newsblur:read");
				}
			}

			if (json_object_object_get_ex(item_obj, "story_date", &node)
					== TRUE)
			{
				const char* pub_date = json_object_get_string(node);
				item.pubDate_ts = parse_date(pub_date);

				char rfc822_date[128];
				strftime(
				    rfc822_date,
				    sizeof(rfc822_date),
				    "%a, %d %b %Y %H:%M:%S %z",
				    gmtime(&item.pubDate_ts));
				item.pubDate = rfc822_date;
			}

			f.items.push_back(item);
		}
	}

	std::sort(
	    f.items.begin(),
	    f.items.end(),
	    [](const rsspp::item& a, const rsspp::item& b) {
	        return a.pubDate_ts > b.pubDate_ts;
	});

	return f;
}

json_object * newsblur_api::query_api(const std::string& endpoint, const std::string* postdata) {

	std::string url = api_location + endpoint;
	std::string data = utils::retrieve_url(url, cfg, "", postdata);

	json_object * result =  json_tokener_parse(data.c_str());
	if (!result)
		LOG(level::WARN, "newsblur_api::query_api: request to %s failed", url);
	return result;
}

}

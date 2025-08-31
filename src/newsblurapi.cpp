#include "newsblurapi.h"

#include <algorithm>
#include <string.h>
#include <time.h>

#include "logger.h"
#include "remoteapi.h"
#include "strprintf.h"
#include "utils.h"

/* json-c 0.13.99 does not define TRUE/FALSE anymore
 * the json-c maintainers replaced them with pure 1/0
 * https://github.com/json-c/json-c/commit/0992aac61f8b
 */
#if defined JSON_C_VERSION_NUM && JSON_C_VERSION_NUM >= ((13 << 8) | 99)
#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE  1
#endif
#endif

#define NEWSBLUR_ITEMS_PER_PAGE 6

using HTTPMethod = newsboat::utils::HTTPMethod;

namespace newsboat {

NewsBlurApi::NewsBlurApi(ConfigContainer& c)
	: RemoteApi(c)
{
	api_location = cfg.get_configvalue("newsblur-url");
	min_pages = (cfg.get_configvalue_as_int("newsblur-min-items") +
			(NEWSBLUR_ITEMS_PER_PAGE + 1)) /
		NEWSBLUR_ITEMS_PER_PAGE;

	if (cfg.get_configvalue_as_filepath("cookie-cache") == Filepath{}) {
		LOG(Level::CRITICAL,
			"NewsBlurApi::NewsBlurApi: No cookie-cache has been "
			"configured the login won't work.");
	}
}

bool NewsBlurApi::authenticate()
{
	json_object* response{};
	json_object* status{};

	std::string auth = retrieve_auth();
	if (auth.empty()) {
		return false;
	}

	response = NewsBlurApi::query_api("/api/login", &auth, HTTPMethod::POST);
	json_object_object_get_ex(response, "authenticated", &status);
	bool result = json_object_get_boolean(status);

	LOG(Level::INFO,
		"NewsBlurApi::authenticate: authentication resulted in %u, cached in %s",
		result,
		cfg.get_configvalue_as_filepath("cookie-cache"));

	return result;
}

std::string NewsBlurApi::retrieve_auth()
{
	Credentials cred = get_credentials("newsblur", "NewsBlur");
	if (cred.user.empty() || cred.pass.empty()) {
		LOG(Level::CRITICAL,
			"NewsBlurApi::retrieve_auth: No user and/or password "
			"set");
		return "";
	}

	return strprintf::fmt("username=%s&password=%s", cred.user, cred.pass);
}

std::vector<TaggedFeedUrl> NewsBlurApi::get_subscribed_urls()
{
	std::vector<TaggedFeedUrl> result;

	json_object* response = query_api("/reader/feeds/", nullptr);

	json_object* feeds{};
	json_object_object_get_ex(response, "feeds", &feeds);

	json_object_iterator it = json_object_iter_begin(feeds);
	json_object_iterator itEnd = json_object_iter_end(feeds);

	json_object* folders{};
	json_object_object_get_ex(response, "folders", &folders);

	std::map<std::string, std::vector<std::string>> feeds_to_tags =
			mk_feeds_to_tags(folders);

	while (!json_object_iter_equal(&it, &itEnd)) {
		const char* feed_id = json_object_iter_peek_name(&it);
		json_object* node{};
		rsspp::Feed current_feed;

		current_feed.rss_version = rsspp::Feed::NEWSBLUR_JSON;

		json_object* feed_json = json_object_iter_peek_value(&it);
		json_object_object_get_ex(feed_json, "feed_title", &node);
		const auto title = json_object_get_string(node);
		if (title != nullptr) {
			current_feed.title = title;
		} else {
			LOG(Level::WARN, "Subscription has no title, so let's call it \"%s\"", feed_id);
			current_feed.title = feed_id;
		}
		json_object_object_get_ex(feed_json, "feed_link", &node);
		if (node) {
			const auto link = json_object_get_string(node);
			if (link == nullptr) {
				LOG(Level::WARN, "Skipping a subscription without a link");
				continue;
			}
			current_feed.link = link;

			known_feeds[feed_id] = current_feed;
			std::string std_feed_id(feed_id);
			std::vector<std::string> tags =
				feeds_to_tags[std_feed_id];
			result.push_back(TaggedFeedUrl(std_feed_id, tags));
		} else {
			LOG(Level::ERROR,
				"NewsBlurApi::get_subscribed_urls: feed fetch "
				"for "
				"%s failed, please check in NewsBlur",
				current_feed.title);
		}

		json_object_iter_next(&it);
	}

	return result;
}

std::map<std::string, std::vector<std::string>> NewsBlurApi::mk_feeds_to_tags(
		json_object* folders)
{
	std::map<std::string, std::vector<std::string>> result;
	array_list* tags = json_object_get_array(folders);
	int tags_len = array_list_length(tags);
	for (int i = 0; i < tags_len; ++i) {
		json_object* tag_to_feed_ids =
			json_object_array_get_idx(folders, i);

		if (!json_object_is_type(tag_to_feed_ids, json_type_object))
			// "folders" array contains not only dictionaries
			// describing folders but also numbers, which are IDs of
			// feeds that don't belong to any folder. This check
			// skips these IDs.
		{
			continue;
		}

		// invariant: `tag_to_feed_ids` is a JSON object

		json_object_object_foreach(tag_to_feed_ids, key, feeds_with_tag_obj) {
			std::string std_key(key);
			array_list* feeds_with_tag_arr = json_object_get_array(feeds_with_tag_obj);
			int feeds_with_tag_len = array_list_length(feeds_with_tag_arr);
			for (int j = 0; j < feeds_with_tag_len; ++j) {
				json_object* feed_id_obj = json_object_array_get_idx(feeds_with_tag_obj, j);
				const auto id = json_object_get_string(feed_id_obj);
				if (id == nullptr) {
					LOG(Level::WARN, "Skipping subscription's tag whose name is a null value");
					continue;
				}
				result[std::string(id)].push_back(std_key);
			}
		}
	}
	return result;
}

void NewsBlurApi::add_custom_headers(curl_slist** /* custom_headers */)
{
	// nothing required
}

bool request_successfull(json_object* payload)
{
	json_object* result{};
	if (json_object_object_get_ex(payload, "result", &result) == FALSE) {
		return false;
	} else {
		const auto result_str = json_object_get_string(result);
		return result_str != nullptr && strcmp("ok", result_str) == 0;
	}
}

bool NewsBlurApi::mark_all_read(const std::string& feed_url)
{
	std::string post_data = strprintf::fmt("feed_id=%s", feed_url);
	json_object* query_result =
		query_api("/reader/mark_feed_as_read", &post_data, HTTPMethod::POST);
	return request_successfull(query_result);
}

bool NewsBlurApi::mark_article_read(const std::string& guid, bool read)
{
	// handle dummy articles
	if (guid.empty()) {
		return false;
	}
	std::string endpoint;
	int separator = guid.find(ID_SEPARATOR);
	std::string feed_id = guid.substr(0, separator);
	std::string article_id =
		guid.substr(separator + sizeof(ID_SEPARATOR) - 1);

	std::string post_data =
		"feed_id=" + feed_id + "&" + "story_id=" + article_id;
	if (read) {
		endpoint = "/reader/mark_story_as_read";
	} else {
		endpoint = "/reader/mark_story_as_unread";
	}

	json_object* query_result = query_api(endpoint, &post_data, HTTPMethod::POST);
	return request_successfull(query_result);
}

bool NewsBlurApi::update_article_flags(const std::string& /* oldflags */,
	const std::string& /* newflags */,
	const std::string& /* guid */)
{
	return false;
}

time_t parse_date(const char* raw)
{
	struct tm tm;
	memset(&tm, 0, sizeof(tm));

	strptime(raw, "%Y-%m-%d %H:%M:%S", &tm);
	return mktime(&tm);
}

rsspp::Feed NewsBlurApi::fetch_feed(const std::string& id)
{
	rsspp::Feed f = known_feeds[id];

	LOG(Level::INFO,
		"NewsBlurApi::fetch_feed: about to fetch %u pages of feed %s",
		min_pages,
		id);

	for (unsigned int i = 1; i <= min_pages; i++) {
		std::string page = std::to_string(i);

		json_object* query_result = query_api(
				"/reader/feed/" + id + "?page=" + page, nullptr);

		if (!query_result) {
			return f;
		}

		json_object* stories{};
		if (json_object_object_get_ex(
				query_result, "stories", &stories) == FALSE) {
			LOG(Level::ERROR,
				"NewsBlurApi::fetch_feed: request returned no "
				"stories");
			return f;
		}

		if (json_object_get_type(stories) != json_type_array) {
			LOG(Level::ERROR,
				"NewsBlurApi::fetch_feed: content is not an "
				"array");
			return f;
		}

		struct array_list* items = json_object_get_array(stories);
		int items_size = array_list_length(items);
		LOG(Level::DEBUG,
			"NewsBlurApi::fetch_feed: %d items",
			items_size);

		for (int i = 0; i < items_size; i++) {
			json_object* item_obj = (json_object*)array_list_get_idx(items, i);

			rsspp::Item item;

			json_object* node{};

			if (json_object_object_get_ex(item_obj, "story_title", &node) == TRUE) {
				const auto title = json_object_get_string(node);
				if (title != nullptr) {
					item.title = title;
				}
			}

			if (json_object_object_get_ex(item_obj, "story_authors", &node) == TRUE) {
				const auto author = json_object_get_string(node);
				if (author != nullptr) {
					item.author = author;
				}
			}

			if (json_object_object_get_ex(item_obj, "story_permalink", &node) == TRUE) {
				const auto link = json_object_get_string(node);
				if (link != nullptr) {
					item.link = link;
				}
			}

			if (json_object_object_get_ex(item_obj, "story_content", &node) == TRUE) {
				const auto content_encoded = json_object_get_string(node);
				if (content_encoded != nullptr) {
					item.content_encoded = content_encoded;
				}
			}

			const char* article_id{};
			if (json_object_object_get_ex(item_obj, "id", &node) == TRUE) {
				article_id = json_object_get_string(node);
			}
			item.guid = id + ID_SEPARATOR + (article_id ? article_id : "");

			if (json_object_object_get_ex(item_obj, "read_status", &node) == TRUE) {
				if (!static_cast<bool>(json_object_get_int(node))) {
					item.labels.push_back("newsblur:unread");
				} else {
					item.labels.push_back("newsblur:read");
				}
			}

			if (json_object_object_get_ex(item_obj, "story_date", &node) == TRUE) {
				const char* pub_date = json_object_get_string(node);
				if (pub_date != nullptr) {
					item.pubDate_ts = parse_date(pub_date);
				} else {
					item.pubDate_ts = ::time(nullptr);
				}

				item.pubDate = utils::mt_strf_localtime(
						"%a, %d %b %Y %H:%M:%S %z",
						item.pubDate_ts);
			}

			f.items.push_back(item);
		}
	}

	std::sort(f.items.begin(),
		f.items.end(),
	[](const rsspp::Item& a, const rsspp::Item& b) {
		return a.pubDate_ts > b.pubDate_ts;
	});

	return f;
}

json_object* NewsBlurApi::query_api(const std::string& endpoint,
	const std::string* body,
	const HTTPMethod method /* = GET */)
{
	std::string url = api_location + endpoint;
	std::string data = utils::retrieve_url(url, cfg, "", body, method);

	json_object* result = json_tokener_parse(data.c_str());
	if (!result)
		LOG(Level::WARN,
			"NewsBlurApi::query_api: request to %s failed",
			url);
	return result;
}

} // namespace newsboat

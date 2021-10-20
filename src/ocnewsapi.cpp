#include "ocnewsapi.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <curl/curl.h>
#include <json-c/json.h>
#include <memory>
#include <time.h>

#include "curlhandle.h"
#include "utils.h"

#define OCNEWS_API "/index.php/apps/news/api/v1-2/"

namespace newsboat {

typedef std::unique_ptr<json_object, decltype(*json_object_put)> JsonUptr;

OcNewsApi::OcNewsApi(ConfigContainer* c)
	: RemoteApi(c)
{
	server = cfg->get_configvalue("ocnews-url");

	if (server.empty())
		LOG(Level::CRITICAL,
			"OcNewsApi::OcNewsApi: No owncloud server set");
}

OcNewsApi::~OcNewsApi() {}

bool OcNewsApi::authenticate()
{
	auth = retrieve_auth();
	if (auth.empty()) {
		return false;
	}
	return query("status");
}

std::string OcNewsApi::retrieve_auth()
{
	Credentials cred = get_credentials("ocnews", "ocNews");
	if (cred.user.empty() || cred.pass.empty()) {
		LOG(Level::CRITICAL,
			"OcNewsApi::retrieve_auth: No user and/or password "
			"set");
		return "";
	}
	return cred.user + ":" + cred.pass;
}

std::vector<TaggedFeedUrl> OcNewsApi::get_subscribed_urls()
{
	std::vector<TaggedFeedUrl> result;
	std::map<long, std::string> folders_map;

	json_object* feeds_query;
	if (!query("feeds", &feeds_query)) {
		return result;
	}
	JsonUptr feeds_uptr(feeds_query, json_object_put);

	json_object* folders_query;
	if (!query("folders", &folders_query)) {
		return result;
	}
	JsonUptr folders_uptr(folders_query, json_object_put);

	json_object* folders;
	json_object_object_get_ex(folders_query, "folders", &folders);

	array_list* folders_list = json_object_get_array(folders);
	int folders_length = folders_list->length;

	for (int i = 0; i < folders_length; i++) {
		json_object* folder =
			static_cast<json_object*>(folders_list->array[i]);
		json_object* node;

		json_object_object_get_ex(folder, "id", &node);
		long folder_id = json_object_get_int(node);

		json_object_object_get_ex(folder, "name", &node);
		folders_map[folder_id] = json_object_get_string(node);
	}

	rsspp::Feed starred;
	starred.title = "Starred";
	starred.link = server;
	starred.rss_version = rsspp::Feed::OCNEWS_JSON;

	known_feeds["Starred"] = std::make_pair(starred, 0);
	result.push_back(TaggedFeedUrl("Starred", std::vector<std::string>()));

	json_object* feeds;
	json_object_object_get_ex(feeds_query, "feeds", &feeds);

	array_list* feeds_list = json_object_get_array(feeds);
	int feeds_length = feeds_list->length;

	for (int i = 0; i < feeds_length; i++) {
		json_object* feed =
			static_cast<json_object*>(feeds_list->array[i]);
		json_object* node;
		rsspp::Feed current_feed;

		current_feed.rss_version = rsspp::Feed::OCNEWS_JSON;

		json_object_object_get_ex(feed, "id", &node);
		long feed_id = json_object_get_int(node);

		json_object_object_get_ex(feed, "title", &node);
		current_feed.title = json_object_get_string(node);

		json_object_object_get_ex(feed, "url", &node);
		current_feed.link = json_object_get_string(node);

		while (known_feeds.find(current_feed.title) !=
			known_feeds.end()) {
			current_feed.title += "*";
		}
		known_feeds[current_feed.title] =
			std::make_pair(current_feed, feed_id);

		json_object_object_get_ex(feed, "folderId", &node);
		long folder_id = json_object_get_int(node);

		std::vector<std::string> tags;
		if (folder_id != 0) {
			tags.push_back(folders_map[folder_id]);
		}

		result.push_back(TaggedFeedUrl(current_feed.title, tags));
	}

	std::sort(++begin(result),
		end(result),
	[](const TaggedFeedUrl& a, const TaggedFeedUrl& b) {
		return a.first < b.first;
	});

	return result;
}

bool OcNewsApi::mark_all_read(const std::string& feedurl)
{
	long id = known_feeds[feedurl].second;

	std::string max = std::to_string(std::numeric_limits<long>::max());
	std::string query =
		"feeds/" + std::to_string(id) + "/read?newestItemId=" + max;

	return this->query(query, nullptr, "{}");
}

bool OcNewsApi::mark_article_read(const std::string& guid, bool read)
{
	std::string query = "items/";
	query += guid.substr(0, guid.find_first_of(":"));

	if (read) {
		query += "/read";
	} else {
		query += "/unread";
	}

	return this->query(query, nullptr, "{}");
}

bool OcNewsApi::mark_articles_read(const std::vector<std::string>& guids)
{
	std::vector<std::string> ids;
	for (const auto& guid : guids) {
		ids.push_back(guid.substr(0, guid.find_first_of(":")));
	}

	const std::string query = "items/read/multiple";
	const std::string id_array = strprintf::fmt("[%s]", utils::join(ids, ","));
	const std::string parameters = strprintf::fmt(R"({"items": %s})", id_array);
	return this->query(query, nullptr, parameters);
}

bool OcNewsApi::update_article_flags(const std::string& oldflags,
	const std::string& newflags,
	const std::string& guid)
{
	const auto feedIdStart = guid.find_first_of(":") + 1;
	const auto feedIdEnd = guid.find_first_of("/");
	const std::string feedId = guid.substr(feedIdStart, feedIdEnd - feedIdStart);

	const std::string ocnewsGuid = guid.substr(guid.find_first_of("/") + 1);
	const std::string guidHash = utils::md5hash(ocnewsGuid);

	std::string query = strprintf::fmt("items/%s/%s", feedId, guidHash);

	std::string star_flag = cfg->get_configvalue("ocnews-flag-star");
	if (star_flag.length() > 0) {
		if (strchr(oldflags.c_str(), star_flag[0]) == nullptr &&
			strchr(newflags.c_str(), star_flag[0]) != nullptr) {
			query += "/star";
		} else if (strchr(oldflags.c_str(), star_flag[0]) != nullptr &&
			strchr(newflags.c_str(), star_flag[0]) == nullptr) {
			query += "/unstar";
		} else {
			return true;
		}
	} else {
		return true;
	}

	return this->query(query, nullptr, "{}");
}

rsspp::Feed OcNewsApi::fetch_feed(const std::string& feed_id)
{
	rsspp::Feed feed = known_feeds[feed_id].first;

	std::string query = "items?";
	query += "type=" +
		std::to_string(known_feeds[feed_id].second != 0 ? 0 : 2);
	query += "&id=" + std::to_string(known_feeds[feed_id].second);

	json_object* response;
	if (!this->query(query, &response)) {
		return feed;
	}
	JsonUptr response_uptr(response, json_object_put);

	json_object* items;
	json_object_object_get_ex(response, "items", &items);
	if (json_object_get_type(items) != json_type_array) {
		LOG(Level::ERROR,
			"OcNewsApi::fetch_feed: items is not an array");
		return feed;
	}

	array_list* list = json_object_get_array(items);
	int array_length = list->length;

	feed.items.clear();

	for (int i = 0; i < array_length; i++) {
		json_object* item_j = static_cast<json_object*>(list->array[i]);
		json_object* node;
		rsspp::Item item;

		json_object_object_get_ex(item_j, "title", &node);
		item.title = json_object_get_string(node);

		json_object_object_get_ex(item_j, "url", &node);
		if (node) {
			item.link = json_object_get_string(node);
		}

		json_object_object_get_ex(item_j, "author", &node);
		item.author = json_object_get_string(node);

		json_object_object_get_ex(item_j, "body", &node);
		item.content_encoded = json_object_get_string(node);

		{
			json_object* type_obj;

			json_object_object_get_ex(
				item_j, "enclosureMime", &type_obj);
			json_object_object_get_ex(
				item_j, "enclosureLink", &node);

			if (type_obj && node) {
				const std::string type =
					json_object_get_string(type_obj);
				if (utils::is_valid_podcast_type(type)) {
					item.enclosure_url =
						json_object_get_string(node);
					item.enclosure_type = std::move(type);
				}
			}
		}

		json_object_object_get_ex(item_j, "id", &node);
		long id = json_object_get_int(node);

		json_object_object_get_ex(item_j, "feedId", &node);
		long f_id = json_object_get_int(node);

		json_object_object_get_ex(item_j, "guid", &node);
		item.guid = std::to_string(id) + ":" + std::to_string(f_id) +
			"/" + json_object_get_string(node);

		json_object_object_get_ex(item_j, "unread", &node);
		bool unread = json_object_get_boolean(node);
		if (unread) {
			item.labels.push_back("ocnews:unread");
		} else {
			item.labels.push_back("ocnews:read");
		}

		json_object_object_get_ex(item_j, "pubDate", &node);
		time_t updated = (time_t)json_object_get_int(node);

		item.pubDate = utils::mt_strf_localtime(
				"%a, %d %b %Y %H:%M:%S %z",
				updated);

		feed.items.push_back(item);
	}

	return feed;
}

void OcNewsApi::add_custom_headers(curl_slist** /* custom_headers */)
{
	// nothing required
}

bool OcNewsApi::query(const std::string& query,
	json_object** result,
	const std::string& post)
{
	CurlHandle handle;

	std::string url = server + OCNEWS_API + query;
	curl_easy_setopt(handle.ptr(), CURLOPT_URL, url.c_str());
	curl_slist* headers = NULL;

	utils::set_common_curl_options(handle, cfg);

	static auto write_fn =
	[](void* buffer, size_t size, size_t nmemb, void* userp) {
		std::string* pbuf = static_cast<std::string*>(userp);
		pbuf->append(
			static_cast<const char*>(buffer), size * nmemb);
		return size * nmemb;
	};
	std::string buff;
	curl_easy_setopt(handle.ptr(), CURLOPT_WRITEFUNCTION, *write_fn);
	curl_easy_setopt(handle.ptr(), CURLOPT_WRITEDATA, &buff);

	if (!post.empty()) {
		curl_easy_setopt(handle.ptr(), CURLOPT_POST, 1);
		curl_easy_setopt(handle.ptr(), CURLOPT_POSTFIELDS, post.c_str());
		curl_easy_setopt(handle.ptr(), CURLOPT_CUSTOMREQUEST, "PUT");

		headers = curl_slist_append(headers, "Content-Type: application/json");
		curl_easy_setopt(handle.ptr(), CURLOPT_HTTPHEADER, headers);
	}

	curl_easy_setopt(handle.ptr(), CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
	curl_easy_setopt(handle.ptr(), CURLOPT_USERPWD, auth.c_str());

	CURLcode res = curl_easy_perform(handle.ptr());

	curl_slist_free_all(headers);

	if (res != CURLE_OK && res != CURLE_HTTP_RETURNED_ERROR) {
		LOG(Level::CRITICAL,
			"OcNewsApi::query: connection error code %i (%s)",
			res,
			curl_easy_strerror(res));
		return false;
	}

	long response_code;
	curl_easy_getinfo(handle.ptr(), CURLINFO_RESPONSE_CODE, &response_code);
	if (response_code != 200) {
		if (response_code == 401)
			LOG(Level::CRITICAL,
				"OcNewsApi::query: authentication error");
		else {
			std::string msg = "OcNewsApi::query: error ";
			msg += response_code;
			LOG(Level::CRITICAL, msg);
		}
		return false;
	}

	if (result) {
		*result = json_tokener_parse(buff.c_str());
	}
	return true;
}

} // namespace newsboat

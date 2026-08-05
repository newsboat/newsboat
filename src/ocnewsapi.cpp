#include "ocnewsapi.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <curl/curl.h>
#include <json-c/json.h>
#include <memory>
#include <time.h>

#include "curldatareceiver.h"
#include "curlhandle.h"
#include "logger.h"
#include "utils.h"

#define OCNEWS_API "/index.php/apps/news/api/v1-2/"

using json = nlohmann::json;

namespace newsboat {

OcNewsApi::OcNewsApi(ConfigContainer& c)
	: RemoteApi(c)
{
	server = cfg.get_configvalue("ocnews-url");

	if (server.empty())
		LOG(Level::CRITICAL,
			"OcNewsApi::OcNewsApi: No owncloud server set");
}

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

	json feeds_query;
	if (!query("feeds", &feeds_query)) {
		return result;
	}

	json folders_query;
	if (!query("folders", &folders_query)) {
		return result;
	}

	try {
		json folders = folders_query.at("folders");

		for (json& folder : folders) {
			const long folder_id = folder["id"];
			const std::string name = folder["name"];

			if (!name.empty()) {
				folders_map[folder_id] = name;
			}
		}

		rsspp::Feed starred;
		starred.title = "Starred";
		starred.link = server;
		starred.rss_version = rsspp::Feed::OCNEWS_JSON;

		known_feeds["Starred"] = std::make_pair(starred, 0);
		result.push_back(TaggedFeedUrl("Starred", std::vector<std::string>()));

		json feeds = feeds_query.at("feeds");

		for (auto& [i_str, feed] : feeds.items()) {
			rsspp::Feed current_feed;

			current_feed.rss_version = rsspp::Feed::OCNEWS_JSON;

			long feed_id = feed["id"];

			if (feed.contains("title") && !feed.at("title").is_null()) {
				current_feed.title = feed.at("title");
			} else {
				LOG(Level::WARN, "Subscription has no title, so let's call it \"%s\"", i_str);
				current_feed.title = std::string("~") + i_str;
			}

			if (feed.contains("url") && !feed.at("url").is_null()) {
				current_feed.link = feed.at("url");
			}

			while (known_feeds.find(current_feed.title) != known_feeds.end()) {
				current_feed.title += "*";
			}
			known_feeds[current_feed.title] = std::make_pair(current_feed, feed_id);

			std::vector<std::string> tags;
			if (feed.contains("folderId") && !feed.at("folderId").is_null()) {
				long folder_id = feed["folderId"];
				tags.push_back(folders_map[folder_id]);
			}

			result.push_back(TaggedFeedUrl(current_feed.title, tags));
		}

		std::sort(++begin(result),
			end(result),
		[](const TaggedFeedUrl& a, const TaggedFeedUrl& b) {
			return a.first < b.first;
		});
	} catch (json::exception& e) {
		LOG(Level::ERROR,
			"OcNewsApi::get_subscribed_urls: failed to parse feed information: %s",
			e.what());
		return result;
	}

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
	std::string star_flag = cfg.get_configvalue("ocnews-flag-star");
	bool success = true;
	if (star_flag.length() > 0) {
		update_flag(oldflags, newflags, star_flag[0], [&](bool added) {
			const auto feedIdStart = guid.find_first_of(':') + 1;
			const auto feedIdEnd = guid.find_first_of('/');
			const auto feedId = guid.substr(feedIdStart, feedIdEnd - feedIdStart);
			const auto ocnewsGuid = guid.substr(feedIdEnd + 1);
			const auto query = strprintf::fmt(
					"items/%s/%s/%s",
					feedId,
					utils::md5hash(ocnewsGuid),
					added ? "star" : "unstar");
			success = this->query(query, nullptr, "{}");
		});
	}
	return success;
}

rsspp::Feed OcNewsApi::fetch_feed(const std::string& feed_id)
{
	rsspp::Feed feed = known_feeds[feed_id].first;

	std::string query = "items?";
	query += "type=" +
		std::to_string(known_feeds[feed_id].second != 0 ? 0 : 2);
	query += "&id=" + std::to_string(known_feeds[feed_id].second);

	json response;
	if (!this->query(query, &response)) {
		return feed;
	}

	try {
		json items = response.at("items");

		feed.items.clear();

		for (auto& [i_str, item_j] : items.items()) {
			rsspp::Item item;

			if (item_j.contains("title") && !item_j.at("title").is_null()) {
				item.title = item_j.at("title");
			}

			if (item_j.contains("url") && !item_j.at("url").is_null()) {
				item.link = item_j.at("url");
			}

			if (item_j.contains("author") && !item_j.at("author").is_null()) {
				item.author = item_j.at("author");
			}

			if (item_j.contains("body") && !item_j.at("body").is_null()) {
				item.content_encoded = item_j.at("body");
			}

			if (
				item_j.contains("enclosureMime")
				&& !item_j.at("enclosureMime").is_null()
				&& item_j.contains("enclosureLink")
				&& !item_j.at("enclosureLink").is_null()
			) {
				const std::string type = item_j.at("enclosureMime");
				const std::string url = item_j.at("enclosureLink");
				item.enclosures.push_back(
				rsspp::Enclosure {
					url,
					type,
					"",
					"",
				}
				);
			}

			long id = item_j["id"];
			long f_id = item_j["feedId"];

			std::string guid = i_str;
			if (item_j.contains("guid") && !item_j.at("guid").is_null()) {
				guid = item_j.at("guid");
			}

			item.guid = std::to_string(id) + ":" + std::to_string(f_id) + "/" + guid;

			bool unread = item_j["unread"];
			if (unread) {
				item.labels.push_back("ocnews:unread");
			} else {
				item.labels.push_back("ocnews:read");
			}

			time_t updated = item_j["pubDate"];

			item.pubDate = utils::mt_strf_localtime(
					"%a, %d %b %Y %H:%M:%S %z",
					updated);

			feed.items.push_back(item);
		}
	} catch (json::exception& e) {
		LOG(Level::ERROR,
			"OcNewsApi::fetch_feed: failed to parse items: %s",
			e.what());
		return feed;
	}


	return feed;
}

void OcNewsApi::add_custom_headers(curl_slist** /* custom_headers */)
{
	// nothing required
}

bool OcNewsApi::query(const std::string& query,
	nlohmann::json* result,
	const std::string& post)
{
	CurlHandle handle;

	std::string url = server + OCNEWS_API + query;
	curl_easy_setopt(handle.ptr(), CURLOPT_URL, url.c_str());
	curl_slist* headers = NULL;

	utils::set_common_curl_options(handle, cfg);

	if (!post.empty()) {
		curl_easy_setopt(handle.ptr(), CURLOPT_POST, 1);
		curl_easy_setopt(handle.ptr(), CURLOPT_POSTFIELDS, post.c_str());
		curl_easy_setopt(handle.ptr(), CURLOPT_CUSTOMREQUEST, "PUT");

		headers = curl_slist_append(headers, "Content-Type: application/json");
		curl_easy_setopt(handle.ptr(), CURLOPT_HTTPHEADER, headers);
	}

	curl_easy_setopt(handle.ptr(), CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
	curl_easy_setopt(handle.ptr(), CURLOPT_USERPWD, auth.c_str());

	auto curlDataReceiver = CurlDataReceiver::register_data_handler(handle);

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
			LOG(Level::CRITICAL,
				"OcNewsApi::query: error %" PRIi64,
				static_cast<int64_t>(response_code));
		}
		return false;
	}

	if (result) {
		const std::string buff = curlDataReceiver->get_data();
		try {
			*result = json::parse(buff);
		} catch (json::parse_error& e) {
			LOG(Level::ERROR,
				"OcNewsApi::query: reply failed to parse: %s",
				buff);
			return false;
		}
	}
	return true;
}

} // namespace newsboat

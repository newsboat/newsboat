#include "freshrssapi.h"

#include <cstring>
#include <curl/curl.h>
#include <json.h>
#include <time.h>
#include <vector>

#include "config.h"
#include "strprintf.h"
#include "utils.h"
#include "rss/feed.h"
#include "3rd-party/json.hpp"

#define FRESHRSS_LOGIN "/accounts/ClientLogin"
#define FRESHRSS_API_PREFIX "/reader/api/0/"
#define FRESHRSS_FEED_PREFIX "/reader/api/0/stream/contents/"

#define FRESHRSS_OUTPUT_SUFFIX "?output=json"

#define FRESHRSS_SUBSCRIPTION_LIST \
	FRESHRSS_API_PREFIX "subscription/list" FRESHRSS_OUTPUT_SUFFIX
#define FRESHRSS_API_MARK_ALL_READ_URL FRESHRSS_API_PREFIX "mark-all-as-read"
#define FRESHRSS_API_EDIT_TAG_URL FRESHRSS_API_PREFIX "edit-tag"
#define FRESHRSS_API_TOKEN_URL FRESHRSS_API_PREFIX "token"

namespace newsboat {

FreshRssApi::FreshRssApi(ConfigContainer* c)
	: RemoteApi(c)
{
    /* server = cfg->get_configvalue("freshrss-url"); */
}

bool FreshRssApi::authenticate()
{
	auth = retrieve_auth();
	LOG(Level::DEBUG, "FreshRssApi::authenticate: Auth = %s", auth);
	return auth != "";
}

static size_t my_write_data(void* buffer, size_t size, size_t nmemb,
	void* userp)
{
	std::string* pbuf = static_cast<std::string*>(userp);
	pbuf->append(static_cast<const char*>(buffer), size * nmemb);
	return size * nmemb;
}

std::string FreshRssApi::retrieve_auth()
{
	CURL* handle = curl_easy_init();
	Credentials cred = get_credentials("freshrss", "FreshRSS");
	if (cred.user.empty() || cred.pass.empty()) {
		return "";
	}

	char* username = curl_easy_escape(handle, cred.user.c_str(), 0);
	char* password = curl_easy_escape(handle, cred.pass.c_str(), 0);

	std::string postcontent = strprintf::fmt(
			"service=reader&Email=%s&Passwd=%s&source=%s%2F%s&accountType="
			"HOSTED_OR_GOOGLE&continue=http://www.google.com/",
			username,
			password,
			PROGRAM_NAME,
			utils::program_version());

	curl_free(username);
	curl_free(password);

	std::string result;

	utils::set_common_curl_options(handle, cfg);
	curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, my_write_data);
	curl_easy_setopt(handle, CURLOPT_WRITEDATA, &result);
	curl_easy_setopt(handle, CURLOPT_POSTFIELDS, postcontent.c_str());
	curl_easy_setopt(handle,
		CURLOPT_URL,
		(cfg->get_configvalue("freshrss-url") + FRESHRSS_LOGIN).c_str());
	curl_easy_perform(handle);
	curl_easy_cleanup(handle);

	for (const auto& line : utils::tokenize(result)) {
		LOG(Level::DEBUG, "FreshRssApi::retrieve_auth: line = %s", line);
		if (line.substr(0, 5) == "Auth=") {
			std::string auth = line.substr(5, line.length() - 5);
			return auth;
		}
	}

	return "";
}

std::vector<TaggedFeedUrl> FreshRssApi::get_subscribed_urls()
{
	std::vector<TaggedFeedUrl> urls;

	CURL* handle = curl_easy_init();
	std::string result;
	curl_slist* custom_headers{};
	add_custom_headers(&custom_headers);
	curl_easy_setopt(handle, CURLOPT_HTTPHEADER, custom_headers);

	utils::set_common_curl_options(handle, cfg);
	curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, my_write_data);
	curl_easy_setopt(handle, CURLOPT_WRITEDATA, &result);
	curl_easy_setopt(handle,
		CURLOPT_URL,
		(cfg->get_configvalue("freshrss-url") + FRESHRSS_SUBSCRIPTION_LIST)
		.c_str());
	curl_easy_perform(handle);
	curl_easy_cleanup(handle);
	curl_slist_free_all(custom_headers);

	LOG(Level::DEBUG,
		"FreshRssApi::get_subscribed_urls: document = %s",
		result);

	json_object* reply = json_tokener_parse(result.c_str());
	if (reply == nullptr) {
		LOG(Level::ERROR,
			"FreshRssApi::get_subscribed_urls: failed to parse "
			"response "
			"as JSON.");
		return urls;
	}

	json_object* subscription_obj{};
	json_object_object_get_ex(reply, "subscriptions", &subscription_obj);
	array_list* subscriptions = json_object_get_array(subscription_obj);

	int len = array_list_length(subscriptions);

	for (int i = 0; i < len; i++) {
		std::vector<std::string> tags;
		json_object* sub =
			json_object_array_get_idx(subscription_obj, i);

		json_object* id_str{};
		json_object_object_get_ex(sub, "id", &id_str);
		const char* id = json_object_get_string(id_str);

		json_object* title_str{};
		json_object_object_get_ex(sub, "title", &title_str);
		const char* title = json_object_get_string(title_str);

		tags.push_back(std::string("~") + title);

		char* escaped_id = curl_easy_escape(handle, id, 0);

		auto url = strprintf::fmt("%s%s%s?n=%u",
				cfg->get_configvalue("freshrss-url"),
				FRESHRSS_FEED_PREFIX,
				escaped_id,
				cfg->get_configvalue_as_int("freshrss-min-items"));
		urls.push_back(TaggedFeedUrl(url, tags));

		curl_free(escaped_id);
	}

	json_object_put(reply);

	return urls;
}

void FreshRssApi::add_custom_headers(curl_slist** custom_headers)
{
	if (auth_header.empty()) {
		auth_header = strprintf::fmt(
				"Authorization: GoogleLogin auth=%s", auth);
	}
	LOG(Level::DEBUG,
		"FreshRssApi::add_custom_headers header = %s",
		auth_header);
	*custom_headers =
		curl_slist_append(*custom_headers, auth_header.c_str());
}

bool FreshRssApi::mark_all_read(const std::string& feedurl)
{
	std::string prefix =
		cfg->get_configvalue("freshrss-url") + FRESHRSS_FEED_PREFIX;
	std::string real_feedurl = feedurl.substr(
			prefix.length(), feedurl.length() - prefix.length());
	std::vector<std::string> elems = utils::tokenize(real_feedurl, "?");
	try {
		real_feedurl = utils::unescape_url(elems[0]);
	} catch (const std::runtime_error& e) {
		LOG(Level::DEBUG,
			"FreshRssApi::mark_all_read: Failed to "
			"unescape_url(%s): %s",
			elems[0],
			e.what());
		return false;
	}
	std::string token = get_new_token();

	std::string postcontent =
		strprintf::fmt("s=%s&T=%s", real_feedurl, token);

	std::string result = post_content(cfg->get_configvalue("freshrss-url") +
			FRESHRSS_API_MARK_ALL_READ_URL,
			postcontent);

	return result == "OK";
}

bool FreshRssApi::mark_article_read(const std::string& guid, bool read)
{
	std::string token = get_new_token();
	return mark_article_read_with_token(guid, read, token);
}

bool FreshRssApi::mark_article_read_with_token(const std::string& guid,
	bool read,
	const std::string& token)
{
	std::string postcontent;

	if (read) {
		postcontent = strprintf::fmt(
				"i=%s&a=user/-/state/com.google/read&r=user/-/state/"
				"com.google/kept-unread&ac=edit&T=%s",
				guid,
				token);
	} else {
		postcontent = strprintf::fmt(
				"i=%s&r=user/-/state/com.google/read&a=user/-/state/"
				"com.google/kept-unread&a=user/-/state/com.google/"
				"tracking-kept-unread&ac=edit&T=%s",
				guid,
				token);
	}

	std::string result = post_content(
			cfg->get_configvalue("freshrss-url") + FRESHRSS_API_EDIT_TAG_URL,
			postcontent);

	LOG(Level::DEBUG,
		"FreshRssApi::mark_article_read_with_token: postcontent = %s "
		"result "
		"= %s",
		postcontent,
		result);

	return result == "OK";
}

std::string FreshRssApi::get_new_token()
{
	CURL* handle = curl_easy_init();
	std::string result;
	curl_slist* custom_headers{};

	utils::set_common_curl_options(handle, cfg);
	add_custom_headers(&custom_headers);
	curl_easy_setopt(handle, CURLOPT_HTTPHEADER, custom_headers);
	curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, my_write_data);
	curl_easy_setopt(handle, CURLOPT_WRITEDATA, &result);
	curl_easy_setopt(handle,
		CURLOPT_URL,
		(cfg->get_configvalue("freshrss-url") + FRESHRSS_API_TOKEN_URL)
		.c_str());
	curl_easy_perform(handle);
	curl_easy_cleanup(handle);
	curl_slist_free_all(custom_headers);

	LOG(Level::DEBUG, "FreshRssApi::get_new_token: token = %s", result);

	return result;
}

bool FreshRssApi::update_article_flags(const std::string& oldflags,
	const std::string& newflags,
	const std::string& guid)
{
	std::string star_flag = cfg->get_configvalue("freshrss-flag-star");
	std::string share_flag = cfg->get_configvalue("freshrss-flag-share");
	bool success = true;

	if (star_flag.length() > 0) {
		if (strchr(oldflags.c_str(), star_flag[0]) == nullptr &&
			strchr(newflags.c_str(), star_flag[0]) != nullptr) {
			success = star_article(guid, true);
		} else if (strchr(oldflags.c_str(), star_flag[0]) != nullptr &&
			strchr(newflags.c_str(), star_flag[0]) == nullptr) {
			success = star_article(guid, false);
		}
	}

	if (share_flag.length() > 0) {
		if (strchr(oldflags.c_str(), share_flag[0]) == nullptr &&
			strchr(newflags.c_str(), share_flag[0]) != nullptr) {
			success = share_article(guid, true);
		} else if (strchr(oldflags.c_str(), share_flag[0]) != nullptr &&
			strchr(newflags.c_str(), share_flag[0]) == nullptr) {
			success = share_article(guid, false);
		}
	}

	return success;
}

bool FreshRssApi::star_article(const std::string& guid, bool star)
{
	std::string token = get_new_token();
	std::string postcontent;

	if (star) {
		postcontent = strprintf::fmt(
				"i=%s&a=user/-/state/com.google/starred&ac=edit&T=%s",
				guid,
				token);
	} else {
		postcontent = strprintf::fmt(
				"i=%s&r=user/-/state/com.google/starred&ac=edit&T=%s",
				guid,
				token);
	}

	std::string result = post_content(
			cfg->get_configvalue("freshrss-url") + FRESHRSS_API_EDIT_TAG_URL,
			postcontent);

	return result == "OK";
}

bool FreshRssApi::share_article(const std::string& guid, bool share)
{
	std::string token = get_new_token();
	std::string postcontent;

	if (share) {
		postcontent = strprintf::fmt(
				"i=%s&a=user/-/state/com.google/broadcast&ac=edit&T=%s",
				guid,
				token);
	} else {
		postcontent = strprintf::fmt(
				"i=%s&r=user/-/state/com.google/broadcast&ac=edit&T=%s",
				guid,
				token);
	}

	std::string result = post_content(
			cfg->get_configvalue("freshrss-url") + FRESHRSS_API_EDIT_TAG_URL,
			postcontent);

	return result == "OK";
}

std::string FreshRssApi::post_content(const std::string& url,
	const std::string& postdata)
{
	std::string result;
	curl_slist* custom_headers{};

	CURL* handle = curl_easy_init();
	utils::set_common_curl_options(handle, cfg);
	add_custom_headers(&custom_headers);
	curl_easy_setopt(handle, CURLOPT_HTTPHEADER, custom_headers);
	curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, my_write_data);
	curl_easy_setopt(handle, CURLOPT_WRITEDATA, &result);
	curl_easy_setopt(handle, CURLOPT_POSTFIELDS, postdata.c_str());
	curl_easy_setopt(handle, CURLOPT_URL, url.c_str());
	curl_easy_perform(handle);
	curl_easy_cleanup(handle);
	curl_slist_free_all(custom_headers);

	LOG(Level::DEBUG,
		"FreshRssApi::post_content: url = %s postdata = %s result = %s",
		url,
		postdata,
		result);

	return result;
}

rsspp::Feed FreshRssApi::fetch_feed(const std::string& id, CURL* cached_handle)
{
    rsspp::Feed feed;
    feed.rss_version = rsspp::Feed::FRESHRSS_JSON;

    const std::string query =
        strprintf::fmt(FRESHRSS_FEED_PREFIX, "feed/", id);

    //const nlohmann::json content = run_op(query, nlohmann::json(), HTTPMethod::GET, cached_handle);

    // From
    CURL* handle;
    if (cached_handle) {
        handle = cached_handle;
        LOG(Level::INFO, "Cached handle");
    } else {
        handle = curl_easy_init();
        LOG(Level::INFO, "No cached handle");
    }
    LOG(Level::INFO, "Feed id: %s", id);
    std::string result;
    curl_slist* custom_headers{};
    add_custom_headers(&custom_headers);
    curl_easy_setopt(handle, CURLOPT_HTTPHEADER, custom_headers);

    utils::set_common_curl_options(handle, cfg);
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, my_write_data);
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, &result);
    curl_easy_setopt(handle,
        CURLOPT_URL,
        id.c_str());
    curl_easy_perform(handle);
    if (!cached_handle) {
        curl_easy_cleanup(handle);
    }
    curl_slist_free_all(custom_headers);

    if (result.empty()) {
        LOG(Level::ERROR,
            "FreshRssApi::fetch_feed: Empty response: %s",
            result);
        return feed;
    }
    nlohmann::json content;
    try {
        content = nlohmann::json::parse(result);
    } catch (nlohmann::json::parse_error& e) {
        LOG(Level::ERROR,
            "FreshRssApi::fetch_feed: reply failed to parse: %s",
            result);
        return feed;
    }


    const nlohmann::json entries = content["items"];
    if (!entries.is_array()) {
        LOG(Level::ERROR,
            "FreshRssApi::fetch_feed: items is not an array");
        return feed;
    }

    LOG(Level::DEBUG,
        "FreshRssApi::fetch_feed: %" PRIu64 " items",
        static_cast<uint64_t>(entries.size()));
    try {
        for (const auto& entry : entries) {
            rsspp::Item item;

            if (!entry["title"].is_null()) {
                item.title = entry["title"];
                LOG(Level::INFO, "Feed title: %s", item.title);
            }

            // No such entry?
            //if (!entry["canonical"].is_null()) {
            //    const auto& canon = entry["canonical"];
            //    if (canon.is_array()) {
            //        if (!canon["href"].is_null()) {
            //            item.link = canon["href"];
            //            LOG(Level::INFO, "Feed link: %s", item.link);
            //        }
            //    }
            //}
            if (!entry["canonical"].is_null()) {
                for (const auto& a : entry["canonical"]) {
                    if (!a["href"].is_null()) {
                        item.link = a["href"];
                        LOG(Level::INFO, "Feed link: %s", item.link);
                        break;
                    }
                }
            }

            if (!entry["author"].is_null()) {
                item.author = entry["author"];
                LOG(Level::INFO, "Feed author: %s", item.author);
            }

            if (!entry["summary"].is_null()) {
                LOG(Level::INFO, "Feed summary");
                for (const auto& a : entry["summary"]) {
                    item.content_encoded = std::string(a);
                }
            }

            // if (!entry["content"].is_null()) {
            //     item.content_encoded = entry["content"];
            //     LOG(Level::INFO, "Feed content: %s", item.content_encoded);
            // }

            /* const int entry_id = entry["id"]; */
            /* item.guid = std::to_string(entry_id); */
            if (!entry["id"].is_null()) {
                item.guid = entry["id"];
                LOG(Level::INFO, "Feed id: %s", item.guid);
            }

            // /* item.pubDate = entry["published"]; */
            if (!entry["published"].is_null()) {
                int pub_time = entry["published"];
                time_t updated = static_cast<time_t>(pub_time);

                item.pubDate = utils::mt_strf_localtime(
                        "%a, %d %b %Y %H:%M:%S %z",
                        updated);
                item.pubDate_ts = pub_time;
            }

            bool unread = true;

            if (!entry["categories"].is_null()) {
                for (const auto& a: entry["categories"]) {
                    /* LOG(Level::INFO, "category %s", a); */
                    if (a == "user/-/state/com.google/read") {
                        unread = false;
                    }
                }
            }

            if (unread) {
                item.labels.push_back("unread");
            } else {
                item.labels.push_back("read");
            }

            /* const std::string status = entry["status"]; */
            /* if (status == "unread") { */
            /*     item.labels.push_back("miniflux:unread"); */
            /* } else { */
            /*     item.labels.push_back("miniflux:read"); */
            /* } */

            feed.items.push_back(item);
        }
    } catch (nlohmann::json::exception& e) {
        LOG(Level::ERROR,
            "Exception occurred while parsing feed: ",
            e.what());
    }

    std::sort(feed.items.begin(),
        feed.items.end(),
    [](const rsspp::Item& a, const rsspp::Item& b) {
        return a.pubDate_ts > b.pubDate_ts;
    });

    return feed;
}

} // namespace newsboat

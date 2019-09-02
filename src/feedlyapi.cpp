#include "feedlyapi.h"

#include <cstring>
#include <curl/curl.h>
#include <unordered_map>
#include <json-c/json.h>

#include "utils.h"

#define MARKER_URI "https://feedly.com/v3/markers"
#define MARKER_COUNT_URI "https://feedly.com/v3/markers/counts"
#define STREAMS_URI "https://feedly.com/v3/streams/contents"
#define PROFILE_URI "https://feedly.com/v3/markers/counts"
#define COLLECTION_URI "https://feedly.com/v3/collections"

namespace newsboat {

FeedlyApi::FeedlyApi(ConfigContainer* c) : RemoteApi(c)
{
	star_flag = cfg->get_configvalue("feedly-flag-star");
	std::string access_token = cfg->get_configvalue("feedly-access-token");
	auth_header = strprintf::fmt("Authorization: Bearer %s", access_token);
}

FeedlyApi::~FeedlyApi() {}

void FeedlyApi::add_custom_headers(curl_slist** custom_headers)
{
	// TODO: versus CURLOPT_XOAUTH2_BEARER with CURLAUTH_BEARER option
	*custom_headers = curl_slist_append(*custom_headers, auth_header.c_str());
	*custom_headers = curl_slist_append(*custom_headers,
			"Accept: application/json");
	*custom_headers = curl_slist_append(*custom_headers,
			"Content-Type: application/json");
}

bool FeedlyApi::mark_article_read(const std::string& guid, bool read)
{
	// TODO: better logging / implement / diff approach?
	LOG(Level::WARN, "FeedlyApi::mark_article_read(%s, %i) not implemented", guid,
		read);
	return true;
}

static size_t my_write_data(void* buffer, size_t size, size_t nmemb,
	void* userp)
{
	// TODO: can we move this to utils? used in all api logic
	std::string* pbuf = static_cast<std::string*>(userp);
	pbuf->append(static_cast<const char*>(buffer), size * nmemb);
	return size * nmemb;
}

std::string FeedlyApi::request_url(const std::string& url,
	const std::string* postdata)
{
	std::string result;
	curl_slist* custom_headers{};
	add_custom_headers(&custom_headers);

	// TODO: log feedly rate limit headers?

	CURL* handle = curl_easy_init();
	utils::set_common_curl_options(handle, cfg);
	curl_easy_setopt(handle, CURLOPT_URL, url.c_str());
	curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, my_write_data);
	curl_easy_setopt(handle, CURLOPT_HTTPHEADER, custom_headers);
	curl_easy_setopt(handle, CURLOPT_WRITEDATA, &result);

	if (postdata != nullptr) {
		curl_easy_setopt(handle, CURLOPT_POST, 1);
		curl_easy_setopt(
			handle, CURLOPT_POSTFIELDS, postdata->c_str());
	}

	// TODO: check curl code or do just return the empty string since writefunction cb not called
	curl_easy_perform(handle);
	curl_easy_cleanup(handle);
	curl_slist_free_all(custom_headers);
	return result;
}

std::unordered_map<std::string, TaggedFeedUrl> FeedlyApi::get_all_feeds()
{

	std::unordered_map<std::string, TaggedFeedUrl> result;
	std::string body = request_url(COLLECTION_URI);
	json_object* reply = json_tokener_parse(body.c_str());
	if (reply == nullptr) {
		LOG(Level::ERROR, "FeedlyApi::get_all_feeds: failed to parse "
			"response as JSON.");
	}

	struct array_list* collections = json_object_get_array(reply);
	int num_cols = array_list_length(collections);
	for (int i = 0; i < num_cols; i++) {
		json_object* col = json_object_array_get_idx(reply, i);

		json_object* node{};
		json_object_object_get_ex(col, "label", &node);
		const char* col_label = json_object_get_string(node);

		json_object* feeds_obj{};
		json_object_object_get_ex(col, "feeds", &feeds_obj);
		struct array_list* feeds = json_object_get_array(feeds_obj);
		int num_feeds = array_list_length(feeds);
		for (int i = 0; i < num_feeds; i++) {
			std::vector<std::string> tags;
			json_object* feed = json_object_array_get_idx(feeds_obj, i);

			json_object* node{};
			json_object_object_get_ex(feed, "id", &node);
			const char* feed_id = json_object_get_string(node);

			json_object_object_get_ex(feed, "title", &node);
			const char* feed_title = json_object_get_string(node);

			tags.push_back(std::string(col_label));
			tags.push_back(std::string("~") + feed_title); // custom feed name
			result[std::string(feed_id)] = TaggedFeedUrl(feed_id, tags);
		}
	}
	return result;
}

std::vector<TaggedFeedUrl> FeedlyApi::get_subscribed_urls()
{
	std::vector<TaggedFeedUrl> urls;
	LOG(Level::INFO, "FeedlyApi::get_subscribed_urls");

	// TODO: use const char*
	std::unordered_map<std::string, TaggedFeedUrl> url_map = get_all_feeds();

	std::string body = request_url(MARKER_COUNT_URI);
	json_object* reply = json_tokener_parse(body.c_str());
	if (reply == nullptr) {
		LOG(Level::ERROR, "FeedlyApi::get_subscribed_urls markers: failed to parse "
			"response as JSON.");
	}
	json_object* root_node{};
	json_object_object_get_ex(reply, "unreadcounts", &root_node);
	struct array_list* counts = json_object_get_array(root_node);
	int n_items = array_list_length(counts);
	for (int i = 0; i < n_items; i++) {
		json_object* c = json_object_array_get_idx(root_node, i);
		json_object* node{};
		json_object_object_get_ex(c, "id", &node);
		const char* feed_id = json_object_get_string(node);

		json_object_object_get_ex(c, "count", &node);
		const long feed_count = json_object_get_int(node);

		// TODO: or cast to string and use the rfind func?
		if (strncmp("feed/", feed_id, strlen("feed/")) == 0 && feed_count > 0) {
			urls.push_back(url_map[std::string(feed_id)]);
		}
	}
	json_object_put(reply);
	return urls;
}

bool FeedlyApi::authenticate()
{
	// hit the profile endpoint to check if user passed valid access token
	std::string result = request_url(PROFILE_URI);
	return result != ""; // TODO: or empty function?
}

rsspp::Feed FeedlyApi::fetch_feed(const std::string& id)
{
	rsspp::Feed f;
	f.rss_version = rsspp::Feed::FEEDLY_JSON;
	LOG(Level::INFO, "FeedlyApi::fetch_feed: fetching feed %s", id);

	// TODO: thoughts on creating this temp handle for this easy_escape?
	CURL* h = curl_easy_init();
	const char* encoded_id = curl_easy_escape(h, id.c_str(), 0);
	// TODO: debug fault with unread false..
	std::string url = strprintf::fmt("%s?count=1000&unreadOnly=true&streamId=%s",
			STREAMS_URI, encoded_id);
	curl_easy_cleanup(h);

	std::string result = request_url(url);
	json_object* reply = json_tokener_parse(result.c_str());
	if (reply == nullptr) {
		LOG(Level::ERROR,
			"FeedlyApi::fetch_feed: failed to parse response as JSON");
	}

	json_object* node{};
	json_object_object_get_ex(reply, "title", &node);
	const char* stream_title = json_object_get_string(node);

	f.link = std::string(url); // TODO: original link under alternate.href
	f.title = std::string(stream_title);

	json_object* items_obj{};
	json_object_object_get_ex(reply, "items", &items_obj);

	struct array_list* items = json_object_get_array(items_obj);
	int num_items = array_list_length(items);
	// items are ordered newest first.
	for (int i = 0; i < num_items; i++) {
		json_object* item_obj = json_object_array_get_idx(items_obj, i);

		rsspp::Item item;
		json_object* node{};

		json_object_object_get_ex(item_obj, "id", &node);
		item.guid = json_object_get_string(node);

		json_object_object_get_ex(item_obj, "title", &node);
		item.title = json_object_get_string(node);

		if (json_object_object_get_ex(item_obj, "author", &node) == true) {
			item.author = json_object_get_string(node);
		}

		json_object_object_get_ex(item_obj, "published", &node);
		const char* pub_ts = json_object_get_string(node);
		item.pubDate_ts = (time_t) strtol(pub_ts, NULL, 10);

		json_object_object_get_ex(item_obj, "unread", &node);
		if (json_object_get_int(node) == 1) {
			item.labels.push_back("feedly:unread");
		} else {
			item.labels.push_back("feedly:read");
		}

		json_object_object_get_ex(item_obj, "alternate", &node);
		json_object* sub_node = json_object_array_get_idx(node, 0);
		json_object_object_get_ex(sub_node, "href", &node);
		item.link = json_object_get_string(node);

		f.items.push_back(item);
	}
	json_object_put(reply);
	return f;
}

bool FeedlyApi::mark_all_read(const std::string& feedurl)
{
	//TODO: can we get the last feed entry guid here (to avoid marking any new)?
	LOG(Level::DEBUG, "FeedlyApi::mark_all_read=%s", feedurl);

	json_object* jobj =  json_object_new_object();
	json_object_object_add(jobj, "type", json_object_new_string("feeds"));
	// skipping asOf attribute since implicitly uses current timestamp...
	json_object_object_add(jobj, "action", json_object_new_string("markAsRead"));

	json_object* jarray = json_object_new_array();
	json_object_array_add(jarray, json_object_new_string(feedurl.c_str()));
	json_object_object_add(jobj,"feedIds", jarray);

	std::string req_data = json_object_to_json_string(jobj);
	request_url(MARKER_URI, &req_data);
	// TODO: response is empty regardless so need access to http status.
	return true;
}

bool FeedlyApi::star_article(const std::string& guid, bool star)
{
	json_object* jobj =  json_object_new_object();
	json_object_object_add(jobj, "type", json_object_new_string("entries"));
	std::string action = star ? "markAsSaved" : "markAsUnsaved";
	json_object_object_add(jobj, "action", json_object_new_string(action.c_str()));

	json_object* jarray = json_object_new_array();
	json_object_array_add(jarray, json_object_new_string(guid.c_str()));
	json_object_object_add(jobj,"entryIds", jarray);

	std::string req_data = json_object_to_json_string(jobj);
	request_url(MARKER_URI, &req_data);
	//TODO: handle error with request_url
	return true;
}

bool FeedlyApi::update_article_flags(const std::string& oldflags,
	const std::string& newflags,
	const std::string& guid)
{
	// TODO: lots of possible integrations..
	LOG(Level::DEBUG, "FeedlyApi::update-article_flags=%s,%s,%s", oldflags,
		newflags, guid);
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
	return success;
}
}

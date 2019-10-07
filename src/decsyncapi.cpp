#include "decsyncapi.h"

#include <algorithm>
#include <ctime>
#include <iomanip>
#include <json.h>
#include <sstream>

#include "logger.h"

namespace newsboat {

struct Extra {
	DecsyncApi* api;
};

DecsyncApi::DecsyncApi(ConfigContainer* c)
	: RemoteApi(c)
{
}

DecsyncApi::~DecsyncApi()
{
	decsync_free(decsync);
}

bool DecsyncApi::authenticate()
{
	std::string decsyncDir = cfg->get_configvalue("decsync-dir");
	char ownAppId[256] = {};
	decsync_get_app_id("newsboat", ownAppId, 256);
	int error = decsync_new(&decsync, decsyncDir.c_str(), "rss", NULL, ownAppId);
	if (error != 0) {
		switch (error) {
			case 1:
				LOG(Level::ERROR, "DecSyncApi::authenticate: Invalid .decsync-info.");
				break;
			case 2:
				LOG(Level::ERROR, "DecSyncApi::authenticate: Unsupported DecSync version.");
				break;
			default:
				LOG(Level::ERROR, "DecSyncApi::authenticate: Unknown error.");
				break;
		}
		return false;
	}

	auto emptyListener = [](const char** path, int len, const char* datetime, const char* key_string, const char* value_string, void* extra_void) {
		(void)path;
		(void)len;
		(void)datetime;
		(void)key_string;
		(void)value_string;
		(void)extra_void;
	};
	std::vector<const char*> path = {"articles", "read"};
	decsync_add_listener(decsync, &path[0], path.size(), emptyListener);
	path = {"articles", "marked"};
	decsync_add_listener(decsync, &path[0], path.size(), emptyListener);
	path = {"feeds", "subscriptions"};
	decsync_add_listener(decsync, &path[0], path.size(), [](const char** path, int len, const char* datetime, const char* key_string, const char* value_string, void* extra_void) {
		(void)path;
		(void)len;
		(void)datetime;
		json_object* key = json_tokener_parse(key_string);
		json_object* value = json_tokener_parse(value_string);
		Extra* extra = static_cast<Extra*>(extra_void);
		const char* feedUrl = json_object_get_string(key);
		bool subscribed = json_object_get_boolean(value);
		extra->api->subscribe(feedUrl, subscribed);
	});
	path = {"feeds", "names"};
	decsync_add_listener(decsync, &path[0], path.size(), emptyListener);
	path = {"feeds", "categories"};
	decsync_add_listener(decsync, &path[0], path.size(), emptyListener);
	path = {"categories", "names"};
	decsync_add_listener(decsync, &path[0], path.size(), emptyListener);
	path = {"categories", "parents"};
	decsync_add_listener(decsync, &path[0], path.size(), emptyListener);

	Extra extra = {this};
	path = {"feeds", "subscriptions"};
	decsync_execute_all_stored_entries_for_path(decsync, &path[0], path.size(), &extra);

	return true;
}

std::vector<TaggedFeedUrl> DecsyncApi::get_subscribed_urls()
{
	Extra extra = {this};
	decsync_execute_all_new_entries(decsync, &extra);
	return subscribed_urls;
}

void DecsyncApi::add_custom_headers(curl_slist** custom_headers)
{
	(void)custom_headers;
}

bool DecsyncApi::mark_all_read(const std::string& feedurl)
{
	(void)feedurl;
	// FIXME: get feed from feedurl
//	std::vector<DecsyncEntryWithPath> entries;
//	for (const auto& item : feed.items()) {
//		add_read_entry(entries, *item, true);
//	}
//	decsync_set_entries(decsync, &entries[0], entries.size());
//	for (DecsyncEntryWithPath entry : entries) {
//		decsync_entry_with_path_free(entry);
//	}
	return false;
}

bool DecsyncApi::mark_article_read(const std::string& guid, bool read)
{
	(void)guid;
	(void)read;
	// FIXME: get item from guid
//	std::vector<DecsyncEntryWithPath> entries;
//	add_read_entry(entries, item, read);
//	decsync_set_entries(decsync, &entries[0], entries.size());
//	for (DecsyncEntryWithPath entry : entries) {
//		decsync_entry_with_path_free(entry);
//	}
	return false;
}

bool DecsyncApi::update_article_flags(const std::string& oldflags,
	const std::string& newflags,
	const std::string& guid)
{
	(void)oldflags;
	(void)newflags;
	(void)guid;
	return false;
}

void DecsyncApi::add_read_entry(std::vector<DecsyncEntryWithPath>& entries, const RssItem& item, bool read)
{
	time_t pubdate = item.pubDate_timestamp();
	tm* datetime = std::gmtime(&pubdate);
	std::stringstream year, month, day;
	year  << std::setw(4) << std::setfill('0') << datetime->tm_year + 1900;
	month << std::setw(2) << std::setfill('0') << datetime->tm_mon + 1;
	day   << std::setw(2) << std::setfill('0') << datetime->tm_mday;
	std::vector<const char*> path = {"articles", "read", year.str().c_str(), month.str().c_str(), day.str().c_str()};
	json_object* key = json_object_new_string(item.guid().c_str());
	json_object* value = json_object_new_boolean(read);
	const char* key_string = json_object_to_json_string(key);
	const char* value_string = json_object_to_json_string(value);
	entries.push_back(decsync_entry_with_path_new(&path[0], path.size(), key_string, value_string));
	json_object_put(key);
	json_object_put(value);
}

void DecsyncApi::subscribe(std::string feedUrl, bool subscribed)
{
	auto feedUrlPos = std::find_if(subscribed_urls.begin(), subscribed_urls.end(), [&feedUrl](const TaggedFeedUrl& taggedUrl) {
		return taggedUrl.first == feedUrl;
	});
	if (subscribed) {
		if (feedUrlPos == subscribed_urls.end()) {
			subscribed_urls.push_back({feedUrl, std::vector<std::string>()});
		}
	} else {
		if (feedUrlPos != subscribed_urls.end()) {
			subscribed_urls.erase(feedUrlPos);
		}
	}
}

} // namespace newsboat

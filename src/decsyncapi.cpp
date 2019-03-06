#include "decsyncapi.h"

#include <algorithm>
#include <ctime>

extern "C" {
#include "vala/decsync.h"
}

#include "rss.h"

namespace newsboat {

DecsyncApi::DecsyncApi(ConfigContainer* c)
	: RemoteApi(c)
{
	std::string decsyncDir = cfg->get_configvalue("decsync-dir");
	auto subscribeWrapper = [](void* api, const char* feedUrl, int subscribed, gpointer /*user_data*/) {
		((DecsyncApi*) api)->subscribe(std::string(feedUrl), subscribed);
	};
	decsync = getDecsync(decsyncDir == "" ? NULL : decsyncDir.c_str(), subscribeWrapper, NULL, NULL);
	subscribed_urls = std::vector<TaggedFeedUrl>();
	executeStoredSubscriptions((Decsync*) decsync, this);
}

DecsyncApi::~DecsyncApi()
{
	g_object_unref((Decsync*) decsync);
}

bool DecsyncApi::authenticate()
{
	return true;
}

std::vector<TaggedFeedUrl> DecsyncApi::get_subscribed_urls()
{
	executeAllNewEntries((Decsync*) decsync, this);
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
	return false;
}

bool DecsyncApi::mark_article_read(const std::string& guid, bool read)
{
	(void)guid;
	(void)read;
	// FIXME: get item from guid
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

void DecsyncApi::mark_feed_read(RssFeed& feed)
{
	for (const auto& item : feed.items()) {
		mark_item_read(*item, true);
	}
}

void DecsyncApi::mark_item_read(const RssItem& item, bool read)
{
	time_t pubdate = item.pubDate_timestamp();
	tm* datetime = std::gmtime(&pubdate);
	int year = datetime->tm_year + 1900;
	int month = datetime->tm_mon + 1;
	int day = datetime->tm_mday;
	markArticleRead((Decsync*) decsync, year, month, day, item.guid().c_str(), read);
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

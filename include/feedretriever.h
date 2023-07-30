#ifndef NEWSBOAT_FEEDRETRIEVER_H_
#define NEWSBOAT_FEEDRETRIEVER_H_

#include <string>

#include "rss/feed.h"

namespace newsboat {

class Cache;
class ConfigContainer;
class CurlHandle;
class RemoteApi;
class RssIgnores;

class FeedRetriever {
public:
	FeedRetriever(ConfigContainer& cfg, Cache& ch, RssIgnores* ign = nullptr,
		RemoteApi* api = nullptr, CurlHandle* easyhandle = nullptr);

	rsspp::Feed retrieve(const std::string& uri);

private:
	void fetch_ttrss(const std::string& feed_id);
	void fetch_newsblur(const std::string& feed_id);
	void fetch_ocnews(const std::string& feed_id);
	void fetch_miniflux(const std::string& feed_id);
	void fetch_freshrss(const std::string& feed_id);
	void download_http(const std::string& uri);
	void get_execplugin(const std::string& plugin);
	void download_filterplugin(const std::string& filter, const std::string& uri);
	void parse_file(const std::string& file);

	ConfigContainer& cfg;
	Cache& ch;
	RssIgnores* ign;
	RemoteApi* api;
	CurlHandle* easyhandle;

	rsspp::Feed f;
};

} // namespace newsboat

#endif /* NEWSBOAT_RSSPARSER_H_ */

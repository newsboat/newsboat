#ifndef NEWSBOAT_FEEDRETRIEVER_H_
#define NEWSBOAT_FEEDRETRIEVER_H_

#include <string>

#include "rss/feed.h"
#include "filepath.h"

namespace newsboat {

class Cache;
class ConfigContainer;
class CurlHandle;
class RemoteApi;
class RssIgnores;

class FeedRetriever {
public:
	FeedRetriever(ConfigContainer& cfg, Cache& ch, CurlHandle& easyhandle,
		RssIgnores* ign = nullptr, RemoteApi* api = nullptr);

	rsspp::Feed retrieve(const std::string& uri);

private:
	rsspp::Feed fetch_ttrss(const std::string& feed_id);
	rsspp::Feed fetch_newsblur(const std::string& feed_id);
	rsspp::Feed fetch_ocnews(const std::string& feed_id);
	rsspp::Feed fetch_miniflux(const std::string& feed_id);
	rsspp::Feed fetch_freshrss(const std::string& feed_id);
	rsspp::Feed fetch_feedbin(const std::string& feed_id);
	rsspp::Feed download_http(const std::string& uri);
	rsspp::Feed get_execplugin(const std::string& plugin);
	rsspp::Feed download_filterplugin(const std::string& filter, const std::string& uri);
	rsspp::Feed parse_file(const newsboat::Filepath& file);

	ConfigContainer& cfg;
	Cache& ch;
	RssIgnores* ign;
	RemoteApi* api;
	CurlHandle& easyhandle;
};

} // namespace newsboat

#endif /* NEWSBOAT_RSSPARSER_H_ */

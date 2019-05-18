#ifndef NEWSBOAT_OCNEWSAPI_H_
#define NEWSBOAT_OCNEWSAPI_H_

#include <json-c/json.h>
#include <map>

#include "remoteapi.h"
#include "rss/feed.h"
#include "urlreader.h"

namespace newsboat {

class OcNewsApi : public RemoteApi {
public:
	explicit OcNewsApi(ConfigContainer* cfg);
	~OcNewsApi() override;
	bool authenticate() override;
	std::vector<TaggedFeedUrl> get_subscribed_urls() override;
	bool mark_all_read(const std::string& feedurl) override;
	bool mark_article_read(const std::string& guid, bool read) override;
	bool update_article_flags(const std::string& oldflags,
		const std::string& newflags,
		const std::string& guid) override;
	void add_custom_headers(curl_slist**) override;
	rsspp::Feed fetch_feed(const std::string& feed_id);

private:
	typedef std::map<std::string, std::pair<rsspp::Feed, long>> FeedMap;
	std::string retrieve_auth();
	bool query(const std::string& query,
		json_object** result = nullptr,
		const std::string& post = "");
	std::string md5(const std::string& str);
	std::string auth;
	std::string server;
	FeedMap known_feeds;
};

class OcNewsUrlReader : public UrlReader {
public:
	OcNewsUrlReader(const std::string& url_file, RemoteApi* a);
	~OcNewsUrlReader() override;
	void write_config() override;
	void reload() override;
	std::string get_source() override;

private:
	std::string file;
	RemoteApi* api;
};

} // namespace newsboat

#endif /* NEWSBOAT_OCNEWSAPI_H_ */

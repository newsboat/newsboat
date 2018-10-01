#ifndef NEWSBOAT_TTRSSAPI_H_
#define NEWSBOAT_TTRSSAPI_H_

#include "3rd-party/json.hpp"
#include "cache.h"
#include "remoteapi.h"
#include "rsspp.h"
#include "urlreader.h"

namespace newsboat {

class TtrssApi : public RemoteApi {
public:
	explicit TtrssApi(ConfigContainer* c);
	~TtrssApi() override;
	bool authenticate() override;
	virtual nlohmann::json run_op(const std::string& op,
		const std::map<std::string, std::string>& args,
		bool try_login = true,
		CURL* Cached_handle = nullptr);
	std::vector<tagged_feedurl> get_subscribed_urls() override;
	void add_custom_headers(curl_slist** custom_headers) override;
	bool mark_all_read(const std::string& feedurl) override;
	bool mark_article_read(const std::string& guid, bool read) override;
	bool update_article_flags(const std::string& oldflags,
		const std::string& newflags,
		const std::string& guid) override;
	rsspp::feed fetch_feed(const std::string& id, CURL* Cached_handle);
	bool update_article(const std::string& guid, int mode, int field);

private:
	void fetch_feeds_per_category(const nlohmann::json& cat,
		std::vector<tagged_feedurl>& feeds);
	bool star_article(const std::string& guid, bool star);
	bool publish_article(const std::string& guid, bool publish);
	tagged_feedurl feed_from_json(const nlohmann::json& jfeed,
		const std::vector<std::string>& tags);
	int parse_category_id(const nlohmann::json& jcatid);
	unsigned int query_api_Level();
	std::string url_to_id(const std::string& url);
	std::string retrieve_sid();
	std::string sid;
	std::string auth_info;
	bool single;
	std::mutex auth_lock;
	int api_Level = -1;
};

class TtrssUrlReader : public UrlReader {
public:
	TtrssUrlReader(const std::string& url_file, RemoteApi* a);
	~TtrssUrlReader() override;
	void write_config() override;
	void reload() override;
	std::string get_source() override;

private:
	std::string file;
	RemoteApi* api;
};

} // namespace newsboat

#endif /* NEWSBOAT_TTRSSAPI_H_ */

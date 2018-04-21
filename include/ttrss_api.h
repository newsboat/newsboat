#ifndef NEWSBOAT_TTRSS_API_H_
#define NEWSBOAT_TTRSS_API_H_

#include "3rd-party/json.hpp"
#include "cache.h"
#include "remote_api.h"
#include "rsspp.h"
#include "urlreader.h"

namespace newsboat {

class ttrss_api : public remote_api {
public:
	explicit ttrss_api(configcontainer* c);
	~ttrss_api() override;
	bool authenticate() override;
	virtual nlohmann::json
	run_op(const std::string& op,
	       const std::map<std::string, std::string>& args,
	       bool try_login = true,
	       CURL* cached_handle = nullptr);
	std::vector<tagged_feedurl> get_subscribed_urls() override;
	void add_custom_headers(curl_slist** custom_headers) override;
	bool mark_all_read(const std::string& feedurl) override;
	bool mark_article_read(const std::string& guid, bool read) override;
	bool update_article_flags(
		const std::string& oldflags,
		const std::string& newflags,
		const std::string& guid) override;
	rsspp::feed fetch_feed(const std::string& id, CURL* cached_handle);
	bool update_article(const std::string& guid, int mode, int field);

private:
	void fetch_feeds_per_category(
		const nlohmann::json& cat,
		std::vector<tagged_feedurl>& feeds);
	bool star_article(const std::string& guid, bool star);
	bool publish_article(const std::string& guid, bool publish);
	tagged_feedurl feed_from_json(
		const nlohmann::json& jfeed,
		const std::vector<std::string>& tags);
	int parse_category_id(const nlohmann::json& jcatid);
	unsigned int query_api_level();
	std::string url_to_id(const std::string& url);
	std::string retrieve_sid();
	std::string sid;
	std::string auth_info;
	bool single;
	std::mutex auth_lock;
	int api_level = -1;
};

class ttrss_urlreader : public urlreader {
public:
	ttrss_urlreader(const std::string& url_file, remote_api* a);
	~ttrss_urlreader() override;
	void write_config() override;
	void reload() override;
	std::string get_source() override;

private:
	std::string file;
	remote_api* api;
};

} // namespace newsboat

#endif /* NEWSBOAT_TTRSS_API_H_ */

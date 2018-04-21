#ifndef NEWSBOAT_OCNEWS_API_H_
#define NEWSBOAT_OCNEWS_API_H_

#include <map>
#include <json-c/json.h>

#include "remote_api.h"
#include "urlreader.h"
#include "rsspp.h"

namespace newsboat {

class ocnews_api : public remote_api {
	public:
		explicit ocnews_api (configcontainer *cfg);
		~ocnews_api() override;
		bool authenticate() override;
		std::vector<tagged_feedurl> get_subscribed_urls() override;
		bool mark_all_read(const std::string& feedurl) override;
		bool mark_article_read(const std::string& guid, bool read) override;
		bool update_article_flags(const std::string& oldflags, const std::string& newflags, const std::string& guid) override;
		void add_custom_headers(curl_slist**) override;
		rsspp::feed fetch_feed(const std::string& feed_id);
	private:
		typedef std::map<std::string, std::pair<rsspp::feed, long>> feedmap;
		std::string retrieve_auth();
		bool query(const std::string& query, json_object** result = nullptr, const std::string& post = "");
		std::string md5(const std::string& str);
		std::string auth;
		std::string server;
		feedmap known_feeds;
};

class ocnews_urlreader : public urlreader {
	public:
		ocnews_urlreader(const std::string& url_file, remote_api * a);
		~ocnews_urlreader() override;
		void write_config() override;
		void reload() override;
		std::string get_source() override;
	private:
		std::string file;
		remote_api * api;
};

}

#endif /* NEWSBOAT_OCNEWS_API_H_ */

#ifndef OCNEWS_API_H
#define OCNEWS_API_H

#include <map>

#include <json-c/json.h>

#include <remote_api.h>
#include <urlreader.h>
#include <rsspp.h>

namespace newsbeuter {

class ocnews_api : public remote_api {
	public:
		explicit ocnews_api (configcontainer *cfg);
		virtual ~ocnews_api();
		virtual bool authenticate();
		virtual std::vector<tagged_feedurl> get_subscribed_urls();
		virtual bool mark_all_read(const std::string& feedurl);
		virtual bool mark_article_read(const std::string& guid, bool read);
		virtual bool update_article_flags(const std::string& oldflags, const std::string& newflags, const std::string& guid);
		virtual void add_custom_headers(curl_slist**);
		rsspp::feed fetch_feed(const std::string& feed_id);
	private:
		typedef std::map<std::string, std::pair<rsspp::feed, long>> feedmap;
		bool query(const std::string& query, json_object** result = nullptr, const std::string& post = "");
		std::string md5(const std::string& str);
		std::string auth;
		std::string server;
		bool verifyhost;
		feedmap known_feeds;
};

class ocnews_urlreader : public urlreader {
	public:
		ocnews_urlreader(const std::string& url_file, remote_api * a);
		virtual ~ocnews_urlreader();
		virtual void write_config();
		virtual void reload();
		virtual std::string get_source();
	private:
		std::string file;
		remote_api * api;
};

}

#endif // OCNEWS_API_H

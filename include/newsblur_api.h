#ifndef NEWSBEUTER_NEWSBLUR_API__H
#define NEWSBEUTER_NEWSBLUR_API__H

#include <remote_api.h>
#include <urlreader.h>
#include <rsspp.h>
#include <json.h>

#define ID_SEPARATOR "/////"

namespace newsbeuter {

typedef std::map<std::string, rsspp::feed> feedmap;

class newsblur_api : public remote_api {
	public:
		newsblur_api(configcontainer * c);
		virtual ~newsblur_api();
		virtual bool authenticate();
		virtual std::vector<tagged_feedurl> get_subscribed_urls();
		virtual void configure_handle(CURL * handle);
		virtual bool mark_all_read(const std::string& feedurl);
		virtual bool mark_article_read(const std::string& guid, bool read);
		virtual bool update_article_flags(const std::string& oldflags, const std::string& newflags, const std::string& guid);
		rsspp::feed fetch_feed(const std::string& id);
		// TODO
	private:
		json_object * query_api(const std::string& url, const std::string* postdata);
		std::string auth_info;
		std::string api_location;
		feedmap known_feeds;
		unsigned int min_pages;
};


class newsblur_urlreader : public urlreader {
	public:
		newsblur_urlreader(const std::string& url_file, remote_api * a);
		virtual ~newsblur_urlreader();
		virtual void write_config();
		virtual void reload();
		virtual std::string get_source();
	private:
		std::string file;
		remote_api * api;
};

}

#endif

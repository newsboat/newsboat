#ifndef NEWSBEUTER_TTRSS_API__H
#define NEWSBEUTER_TTRSS_API__H

#include <rsspp.h>
#include <remote_api.h>
#include <urlreader.h>
#include <cache.h>

namespace newsbeuter {

class ttrss_api : public remote_api {
	public:
		ttrss_api(configcontainer * c);
		virtual ~ttrss_api();
		virtual bool authenticate();
		virtual struct json_object * run_op(const std::string& op, const std::map<std::string, std::string>& args,
						    bool try_login = true);
		virtual std::vector<tagged_feedurl> get_subscribed_urls();
		virtual void configure_handle(CURL * handle);
		virtual bool mark_all_read(const std::string& feedurl);
		virtual bool mark_article_read(const std::string& guid, bool read);
		virtual bool update_article_flags(const std::string& oldflags, const std::string& newflags, const std::string& guid);
		rsspp::feed fetch_feed(const std::string& id);
	private:
		void fetch_feeds_per_category(struct json_object * cat, std::vector<tagged_feedurl>& feeds);
		bool star_article(const std::string& guid, bool star);
		bool publish_article(const std::string& guid, bool publish);
		bool update_article(const std::string& guid, int mode, int field);
		std::string url_to_id(const std::string& url);
		std::string retrieve_sid();
		std::string sid;
		std::string auth_info;
		const char * auth_info_ptr;
		bool single;
		mutex auth_lock;
};

class ttrss_urlreader : public urlreader {
	public:
		ttrss_urlreader(configcontainer * c, const std::string& url_file, remote_api * a);
		virtual ~ttrss_urlreader();
		virtual void write_config();
		virtual void reload();
		virtual std::string get_source();
	private:
		configcontainer * cfg;
		std::string file;
		remote_api * api;
};

}

#endif

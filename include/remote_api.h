#ifndef NEWSBEUTER_REMOTE_API__H
#define NEWSBEUTER_REMOTE_API__H

#include <configcontainer.h>
#include <string>
#include <utility>
#include <vector>
#include <curl/curl.h>

namespace newsbeuter {

typedef std::pair<std::string, std::vector<std::string>> tagged_feedurl;

class remote_api {
	public:
		remote_api(configcontainer * c) : cfg(c) { }
		virtual ~remote_api() { }
		virtual bool authenticate() = 0;
		virtual std::vector<tagged_feedurl> get_subscribed_urls() = 0;
		virtual void configure_handle(CURL * handle) = 0;
		virtual bool mark_all_read(const std::string& feedurl) = 0;
		virtual bool mark_article_read(const std::string& guid, bool read) = 0;
		virtual bool update_article_flags(const std::string& oldflags, const std::string& newflags, const std::string& guid) = 0;
		// TODO
	protected:
		configcontainer * cfg;
};

}

#endif

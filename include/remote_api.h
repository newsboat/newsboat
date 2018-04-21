#ifndef NEWSBOAT_REMOTE_API_H_
#define NEWSBOAT_REMOTE_API_H_

#include <string>
#include <utility>
#include <vector>
#include <curl/curl.h>

#include "configcontainer.h"

namespace newsboat {

typedef std::pair<std::string, std::vector<std::string>> tagged_feedurl;

typedef struct {
	std::string user;
	std::string pass;
} credentials;

class remote_api {
	public:
		explicit remote_api(configcontainer * c) : cfg(c) { }
		virtual ~remote_api() { }
		virtual bool authenticate() = 0;
		virtual std::vector<tagged_feedurl> get_subscribed_urls() = 0;
		virtual void add_custom_headers(curl_slist** custom_headers) = 0;
		virtual bool mark_all_read(const std::string& feedurl) = 0;
		virtual bool mark_article_read(const std::string& guid, bool read) = 0;
		virtual bool update_article_flags(const std::string& oldflags, const std::string& newflags, const std::string& guid) = 0;
		static const std::string read_password(const std::string& file);
		static const std::string eval_password(const std::string& cmd);
		// TODO
	protected:
		configcontainer * cfg;
		credentials get_credentials(const std::string& scope, const std::string& name);
};

}

#endif /* NEWSBOAT_REMOTE_API_H_ */

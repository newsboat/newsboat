#ifndef NEWSBOAT_RSSPPPARSER_H_
#define NEWSBOAT_RSSPPPARSER_H_

#include <curl/curl.h>
#include <libxml/parser.h>
#include <string>

#include "remoteapi.h"
#include "feed.h"

namespace newsboat {

class CurlHandle;

}

namespace rsspp {

class Parser {
public:
	Parser(unsigned int timeout = 30,
		const std::string& user_agent = "",
		const std::string& proxy = "",
		const std::string& proxy_auth = "",
		curl_proxytype proxy_type = CURLPROXY_HTTP,
		const bool ssl_verify = true);
	~Parser();
	Feed parse_url(const std::string& url,
		newsboat::CurlHandle& easyhandle,
		time_t lastmodified = 0,
		const std::string& etag = "",
		newsboat::RemoteApi* api = 0,
		const std::string& cookie_cache = "");
	Feed parse_url(const std::string& url,
		time_t lastmodified = 0,
		const std::string& etag = "",
		newsboat::RemoteApi* api = 0,
		const std::string& cookie_cache = "");
	Feed parse_buffer(const std::string& buffer,
		const std::string& url = "");
	Feed parse_file(const std::string& filename);
	time_t get_last_modified()
	{
		return lm;
	}
	const std::string& get_etag()
	{
		return et;
	}

	static void global_init();
	static void global_cleanup();

private:
	Feed parse_xmlnode(xmlNode* node);
	unsigned int to;
	const std::string ua;
	const std::string prx;
	const std::string prxauth;
	curl_proxytype prxtype;
	const bool verify_ssl;
	xmlDocPtr doc;
	time_t lm;
	std::string et;
};

} // namespace rsspp

#endif /* NEWSBOAT_RSSPPPARSER_H_ */

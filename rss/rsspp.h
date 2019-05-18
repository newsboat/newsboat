#ifndef NEWSBOAT_RSSPP_H_
#define NEWSBOAT_RSSPP_H_

#include <curl/curl.h>
#include <exception>
#include <libxml/parser.h>
#include <string>
#include <vector>

#include "remoteapi.h"
#include "item.h"

namespace rsspp {

class Feed {
public:
	enum Version {
		UNKNOWN = 0,
		RSS_0_91,
		RSS_0_92,
		RSS_1_0,
		RSS_2_0,
		ATOM_0_3,
		ATOM_1_0,
		RSS_0_94,
		ATOM_0_3_NONS,
		TTRSS_JSON,
		NEWSBLUR_JSON,
		OCNEWS_JSON
	};

	Feed()
		: rss_version(UNKNOWN)
	{
	}

	std::string encoding;

	Version rss_version;
	std::string title;
	std::string title_type;
	std::string description;
	std::string link;
	std::string language;
	std::string managingeditor;
	std::string dc_creator;
	std::string pubDate;

	std::vector<Item> items;
};

class Exception : public std::exception {
public:
	explicit Exception(const std::string& errmsg = "");
	~Exception() throw() override;
	const char* what() const throw() override;

private:
	std::string emsg;
};

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
		time_t lastmodified = 0,
		const std::string& etag = "",
		newsboat::RemoteApi* api = 0,
		const std::string& cookie_cache = "",
		CURL* ehandle = 0);
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

#endif /* NEWSBOAT_RSSPP_H_ */

#ifndef NEWSBOAT_RSSPP_H_
#define NEWSBOAT_RSSPP_H_

#include <string>
#include <vector>
#include <exception>
#include <libxml/parser.h>
#include <curl/curl.h>

#include "remote_api.h"

namespace rsspp {

enum version { UNKNOWN = 0, RSS_0_91, RSS_0_92, RSS_1_0, RSS_2_0, ATOM_0_3, ATOM_1_0, RSS_0_94, ATOM_0_3_NONS, TTRSS_JSON, NEWSBLUR_JSON, OCNEWS_JSON };

class item {
	public:
		item() : guid_isPermaLink(false), pubDate_ts(0) {}

		std::string title;
		std::string title_type;
		std::string link;
		std::string description;
		std::string description_type;

		std::string author;
		std::string author_email;

		std::string pubDate;
		std::string guid;
		bool guid_isPermaLink;

		std::string enclosure_url;
		std::string enclosure_type;

		// extensions:
		std::string content_encoded;
		std::string itunes_summary;

		// Atom-specific:
		std::string base;
		std::vector<std::string> labels;

		// only required for ttrss support:
		time_t pubDate_ts;
};

class feed {
	public:
		feed() : rss_version(UNKNOWN) {}

		std::string encoding;

		version rss_version;
		std::string title;
		std::string title_type;
		std::string description;
		std::string link;
		std::string language;
		std::string managingeditor;
		std::string dc_creator;
		std::string pubDate;

		std::vector<item> items;
};

class exception : public std::exception {
	public:
		explicit exception(const std::string& errmsg = "");
		~exception() throw() override;
		const char* what() const throw() override;
	private:
		std::string emsg;
};

class parser {
	public:
		parser(
				unsigned int timeout = 30,
				const std::string& user_agent = "",
				const std::string& proxy = "",
				const std::string& proxy_auth = "",
				curl_proxytype proxy_type = CURLPROXY_HTTP,
				const bool ssl_verify = true);
		~parser();
		feed parse_url(const std::string& url, time_t lastmodified = 0, const std::string& etag = "", newsboat::remote_api * api = 0, const std::string& cookie_cache = "", CURL *ehandle = 0);
		feed parse_buffer(const std::string& buffer, const std::string& url = "");
		feed parse_file(const std::string& filename);
		time_t get_last_modified() {
			return lm;
		}
		const std::string& get_etag() {
			return et;
		}

		static void global_init();
		static void global_cleanup();
	private:

		feed parse_xmlnode(xmlNode * node);
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

}

#endif /* NEWSBOAT_RSSPP_H_ */

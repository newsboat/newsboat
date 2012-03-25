/* rsspp - Copyright (C) 2008-2012 Andreas Krennmair <ak@newsbeuter.org>
 * Licensed under the MIT/X Consortium License. See file LICENSE
 * for more information.
 */

#ifndef RSSPP_H
#define RSSPP_H

#include <string>
#include <vector>
#include <exception>
#include <libxml/parser.h>
#include <curl/curl.h>
#include <remote_api.h>

namespace rsspp {

enum version { UNKNOWN = 0, RSS_0_91, RSS_0_92, RSS_1_0, RSS_2_0, ATOM_0_3, ATOM_1_0, RSS_0_94, ATOM_0_3_NONS, TTRSS_JSON };

struct item {
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

struct feed {
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
		exception(const std::string& errmsg = "");
		~exception() throw();
		virtual const char* what() const throw();
	private:
		std::string emsg;
};

class parser {
	public:
		parser(unsigned int timeout = 30, const char * user_agent = 0, const char * proxy = 0, const char * proxy_auth = 0, curl_proxytype proxy_type = CURLPROXY_HTTP);
		~parser();
		feed parse_url(const std::string& url, time_t lastmodified = 0, const std::string& etag = "", newsbeuter::remote_api * api = 0, const std::string& cookie_cache = "", CURL *ehandle = 0);
		feed parse_buffer(const char * buffer, size_t size, const char * url = NULL);
		feed parse_file(const std::string& filename);
		time_t get_last_modified() { return lm; }
		const std::string& get_etag() { return et; }

		static void global_init();
		static void global_cleanup();
	private:

		feed parse_xmlnode(xmlNode * node);
		unsigned int to;
		const char * ua;
		const char * prx;
		const char * prxauth;
		curl_proxytype prxtype;
		xmlDocPtr doc;
		time_t lm;
		std::string et;
};

}


#endif

/* rsspp - Copyright (C) 2008 Andreas Krennmair <ak@newsbeuter.org>
 * Licensed under the MIT/X Consortium License. See file LICENSE
 * for more information.
 */

#ifndef RSSPP_H
#define RSSPP_H

#include <string>
#include <vector>
#include <exception>
#include <libxml/parser.h>

namespace rsspp {

enum version { UNKNOWN = 0, RSS_0_91, RSS_0_92, RSS_1_0, RSS_2_0, ATOM_0_3, ATOM_1_0 };

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

	std::string enclosure;
	std::string enclosure_url;
	ssize_t enclosure_length;
	std::string enclosure_type;

	// extensions:
	std::string content_encoded;
	std::string atom_content;
	std::string itunes_summary;
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
	std::string pubDate;

	std::vector<item> items;
};

class exception : public std::exception {
	public:
		exception(unsigned int error_code = 0, const std::string& errmsg = "");
		~exception() throw();
		virtual const char* what() const throw();
	private:
		unsigned int ecode;
		std::string emsg;
		std::string buf;
};

class parser {
	public:
		parser(unsigned int timeout = 30, const char * user_agent = 0, const char * proxy = 0, const char * proxy_auth = 0);
		~parser();
		feed parse_url(const std::string& url);
		feed parse_buffer(const char * buffer, size_t size);
		feed parse_file(const std::string& filename);

		static void global_init();
		static void global_cleanup();
	private:

		feed parse_xmlnode(xmlNode * node);
		unsigned int to;
		const char * ua;
		const char * prx;
		const char * prxauth;
};

}


#endif

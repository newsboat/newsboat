#ifndef RSS_PARSER__H
#define RSS_PARSER__H

#include <string>

#include <_mrss.h>
#include <rss.h>

namespace newsbeuter {

	class configcontainer;
	class cache;

	class rss_parser {
		public:
			rss_parser(const char * uri, cache * c, configcontainer *, rss_ignores * ii);
			~rss_parser();
			rss_feed parse();
			bool check_and_update_lastmodified();
		private:
			void replace_newline_characters(std::string& str);
			mrss_options_t * create_mrss_options();
			std::string render_xhtml_title(const std::string& title, const std::string& link);
			time_t parse_date(const std::string& datestr);
			unsigned int monthname_to_number(const std::string& mon);
			void set_rtl(rss_feed& feed, const char * lang);

			std::string my_uri;
			cache * ch;
			configcontainer *cfgcont;
			mrss_t * mrss;
			rss_ignores * ign;
	};

}

#endif

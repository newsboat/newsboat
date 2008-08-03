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
			int correct_year(int year);

			void retrieve_uri(const std::string& uri);
			void download_http(const std::string& uri);
			void get_execplugin(const std::string& plugin);
			void download_filterplugin(const std::string& filter, const std::string& uri);

			void check_and_log_error();

			void fill_feed_fields(rss_feed& feed, const char * encoding);
			void fill_feed_items(rss_feed& feed, const char * encoding);

			void set_item_title(rss_feed& feed, rss_item& x, mrss_item_t * item, const char * encoding);
			void set_item_author(rss_item& x, mrss_item_t * item, const char * encoding);
			void set_item_content(rss_item& x, mrss_item_t * item, const char * encoding);
			void set_item_enclosure(rss_item& x, mrss_item_t * item);
			std::string get_guid(mrss_item_t * item);

			void add_item_to_feed(rss_feed& feed, rss_item& item);

			void handle_content_encoded(rss_item& x, mrss_item_t * item, const char * encoding);
			void handle_atom_content(rss_item& x, mrss_item_t * item, const char * encoding);
			void handle_itunes_summary(rss_item& x, mrss_item_t * item, const char * encoding);

			std::string my_uri;
			cache * ch;
			configcontainer *cfgcont;
			mrss_t * mrss;
			mrss_error_t err;
			bool skip_parsing;
			rss_ignores * ign;
			CURLcode ccode;
	};

}

#endif

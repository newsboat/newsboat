#ifndef NEWSBEUTER_RSS__H
#define NEWSBEUTER_RSS__H

#include <string>
#include <vector>

#include <configcontainer.h>


extern "C" {
#include <mrss.h>
}

namespace newsbeuter {
	
	class cache;

	class rss_item {
		public:
			rss_item(cache * c) : unread_(true), ch(c) { }
			~rss_item() { }
			
			inline const std::string& title() const { return title_; }
			void set_title(const std::string& t);
			
			inline const std::string& link() const { return link_; }
			void set_link(const std::string& l);
			
			inline const std::string& author() const { return author_; }
			void set_author(const std::string& a);
			
			inline const std::string& description() const { return description_; }
			void set_description(const std::string& d);
			
			std::string pubDate() const;
			
			inline time_t pubDate_timestamp() const {
				return pubDate_;
			}
			void set_pubDate(time_t t);
			
			inline const std::string& guid() const { return guid_; }
			void set_guid(const std::string& g);
			
			inline bool unread() const { return unread_; }
			void set_unread(bool u);
			
			inline void set_cache(cache * c) { ch = c; }
			inline void set_feedurl(const std::string& f) { feedurl_ = f; }
			
			inline const std::string& feedurl() const { return feedurl_; }
			
		private:
			std::string title_;
			std::string link_;
			std::string author_;
			std::string description_;
			time_t pubDate_;
			std::string guid_;
			std::string feedurl_;
			bool unread_;
			cache * ch;
	};

	class rss_feed {
		public:
			rss_feed(cache * c) : ch(c) { }
			~rss_feed() { }
			inline const std::string& title() const { return title_; }
			inline void set_title(const std::string& t) { title_ = t; }
			
			inline const std::string& description() const { return description_; }
			inline void set_description(const std::string& d) { description_ = d; }
			
			inline const std::string& link() const { return link_; }
			inline void set_link(const std::string& l) { link_ = l; }
			
			inline std::string pubDate() const { return "TODO"; }
			inline void set_pubDate(time_t t) { pubDate_ = t; }
			
			inline std::vector<rss_item>& items() { return items_; }
			
			inline const std::string& rssurl() const { return rssurl_; }
			inline void set_rssurl(const std::string& u) { rssurl_ = u; }
			
			unsigned int unread_item_count() const;

			void set_tags(const std::vector<std::string>& tags);
			bool matches_tag(const std::string& tag);
			std::string get_tags();

		private:
			std::string title_;
			std::string description_;
			std::string link_;
			time_t pubDate_;
			std::string rssurl_;
			std::vector<rss_item> items_;
			std::vector<std::string> tags_;
			
			cache * ch;
	};

	class rss_parser {
		public:
			rss_parser(const char * uri, cache * c, configcontainer *);
			~rss_parser();
			rss_feed parse();
			static time_t parse_date(const std::string& datestr);
		private:
			std::string my_uri;
			cache * ch;
			configcontainer *cfgcont;
			mrss_t * mrss;
	};

}


#endif

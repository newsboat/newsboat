#ifndef NOOS_RSS__H
#define NOOS_RSS__H

#include <string>
#include <vector>


extern "C" {
#include <mrss.h>
}

namespace noos {
	
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
			
			inline const std::string& pubDate() const { return pubDate_; }
			void set_pubDate(const std::string& d);
			
			inline const std::string& guid() const { return guid_; }
			void set_guid(const std::string& g);
			
			inline bool unread() const { return unread_; }
			void set_unread(bool u);
			
			inline void set_cache(cache * c) { ch = c; }
			inline void set_feedurl(const std::string& f) { feedurl = f; }
			
		private:
			std::string title_;
			std::string link_;
			std::string author_;
			std::string description_;
			std::string pubDate_;
			std::string guid_;
			std::string feedurl;
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
			
			inline const std::string& pubDate() const { return pubDate_; }
			inline void set_pubDate(const std::string& p) { pubDate_ = p; }
			
			inline std::vector<rss_item>& items() { return items_; }
			
			inline const std::string& rssurl() const { return rssurl_; }
			inline void set_rssurl(const std::string& u) { rssurl_ = u; }

		private:
			std::string title_;
			std::string description_;
			std::string link_;
			std::string pubDate_;
			std::string rssurl_;
			std::vector<rss_item> items_;
			
			cache * ch;
	};

	class rss_parser {
		public:
			rss_parser(const char * uri, cache * c);
			~rss_parser();
			rss_feed parse();
		private:
			std::string my_uri;
			cache * ch;
			mrss_t * mrss;
	};

}


#endif

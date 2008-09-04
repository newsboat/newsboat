#ifndef NEWSBEUTER_RSS__H
#define NEWSBEUTER_RSS__H

#include <string>
#include <vector>

#include <configcontainer.h>
#include <matcher.h>

#include <tr1/memory>

extern "C" {
#include <_mrss.h>
}

namespace newsbeuter {

	typedef std::pair<std::string, matcher *> feedurl_expr_pair;
	
	class cache;
	class rss_feed;

	class rss_item : public matchable {
		public:
			rss_item(cache * c) : unread_(true), ch(c), enqueued_(false), deleted_(0) { }
			~rss_item() { }
			
			std::string title() const;
			std::string title_raw() const { return title_; }
			void set_title(const std::string& t);
			
			inline const std::string& link() const { return link_; }
			void set_link(const std::string& l);
			
			std::string author() const;
			std::string author_raw() const { return author_; }
			void set_author(const std::string& a);
		 	
			std::string description() const;
			std::string description_raw() const { return description_; }
			void set_description(const std::string& d);
			
			std::string pubDate() const;
			
			inline time_t pubDate_timestamp() const {
				return pubDate_;
			}
			void set_pubDate(time_t t);

			bool operator<(const rss_item& item) const { return item.pubDate_ < this->pubDate_; } // new items come first
			
			inline const std::string& guid() const { return guid_; }
			void set_guid(const std::string& g);
			
			inline bool unread() const { return unread_; }
			void set_unread(bool u);
			void set_unread_nowrite(bool u);
			void set_unread_nowrite_notify(bool u, bool notify);
			
			inline void set_cache(cache * c) { ch = c; }
			inline void set_feedurl(const std::string& f) { feedurl_ = f; }
			
			inline const std::string& feedurl() const { return feedurl_; }

			inline const std::string& enclosure_url() const { return enclosure_url_; }
			inline const std::string& enclosure_type() const { return enclosure_type_; }

			void set_enclosure_url(const std::string& url);
			void set_enclosure_type(const std::string& type);

			inline bool enqueued() { return enqueued_; }
			inline void set_enqueued(bool v) { enqueued_ = v; }

			inline const std::string& flags() const { return flags_; }
			void set_flags(const std::string& ff) { flags_ = ff; sort_flags(); }
			void update_flags();
			void sort_flags();

			virtual bool has_attribute(const std::string& attribname);
			virtual std::string get_attribute(const std::string& attribname);

			void set_feedptr(std::tr1::shared_ptr<rss_feed> ptr);
			inline std::tr1::shared_ptr<rss_feed> get_feedptr() { return feedptr; }

			inline bool deleted() const { return deleted_; }
			inline void set_deleted(bool b) { deleted_ = b; }

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
			std::string enclosure_url_;
			std::string enclosure_type_;
			bool enqueued_;
			std::string flags_;
			std::tr1::shared_ptr<rss_feed> feedptr;
			bool deleted_;
	};

	class rss_feed : public matchable {
		public:
			rss_feed(cache * c) : ch(c), empty(true), is_rtl_(false) { }
			rss_feed() : ch(NULL), empty(true), is_rtl_(false) { }
			~rss_feed() { }
			std::string title_raw() const { return title_; }
			std::string title() const;
			inline void set_title(const std::string& t) { title_ = t; }
			
			std::string description_raw() const { return description_; }
			std::string description() const;
			inline void set_description(const std::string& d) { description_ = d; }
			
			inline const std::string& link() const { return link_; }
			inline void set_link(const std::string& l) { link_ = l; }
			
			inline std::string pubDate() const { return "TODO"; }
			inline void set_pubDate(time_t t) { pubDate_ = t; }
			
			inline std::vector<std::tr1::shared_ptr<rss_item> >& items() { return items_; }

			rss_item& get_item_by_guid(const std::string& guid);
			
			inline const std::string& rssurl() const { return rssurl_; }
			void set_rssurl(const std::string& u);
			
			unsigned int unread_item_count() const;

			void set_tags(const std::vector<std::string>& tags);
			bool matches_tag(const std::string& tag);
			std::string get_tags();

			virtual bool has_attribute(const std::string& attribname);
			virtual std::string get_attribute(const std::string& attribname);

			void update_items(std::vector<std::tr1::shared_ptr<rss_feed> >& feeds);

			inline void set_query(const std::string& s) { query = s; }

			bool is_empty() { return empty; }
			void set_empty(bool t) { empty = t; }

			void sort(const std::string& method);

			void remove_old_deleted_items();

			void purge_deleted_items();

			inline void set_rtl(bool b) { is_rtl_ = b; }
			inline bool is_rtl() { return is_rtl_; }

		private:
			std::string title_;
			std::string description_;
			std::string link_;
			time_t pubDate_;
			std::string rssurl_;
			std::vector<std::tr1::shared_ptr<rss_item> > items_;
			std::vector<std::string> tags_;
			std::string query;
			
			cache * ch;

			bool empty;
			bool is_rtl_;
	};

	class rss_ignores : public config_action_handler {
		public:
			rss_ignores() { }
			virtual ~rss_ignores();
			virtual action_handler_status handle_action(const std::string& action, const std::vector<std::string>& params);
			bool matches(rss_item * item);
			bool matches_lastmodified(const std::string& url);
			bool matches_resetunread(const std::string& url);
		private:
			std::vector<feedurl_expr_pair> ignores;
			std::vector<std::string> ignores_lastmodified;
			std::vector<std::string> resetflag;
	};

}


#endif

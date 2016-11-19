#ifndef NEWSBEUTER_RSS__H
#define NEWSBEUTER_RSS__H

#include <string>
#include <vector>

#include <configcontainer.h>
#include <matcher.h>
#include <mutex>
#include <utils.h>

#include <memory>
#include <unordered_map>

namespace newsbeuter {

typedef std::pair<std::string, matcher *> feedurl_expr_pair;

enum class dl_status { SUCCESS, TO_BE_DOWNLOADED, DURING_DOWNLOAD, DL_ERROR };

class cache;
class rss_feed;

class rss_item : public matchable {
	public:
		explicit rss_item(cache * c);
		~rss_item();

		std::string title() const;
		std::string title_raw() const {
			return title_;
		}
		void set_title(const std::string& t);

		inline const std::string& link() const {
			return link_;
		}
		void set_link(const std::string& l);

		std::string author() const;
		std::string author_raw() const {
			return author_;
		}
		void set_author(const std::string& a);

		std::string description() const;
		std::string description_raw() const {
			return description_;
		}
		void set_description(const std::string& d);

		inline unsigned int size() const {
			return size_;
		}
		void set_size(unsigned int size);

		std::string length() const;
		std::string pubDate() const;

		inline time_t pubDate_timestamp() const {
			return pubDate_;
		}
		void set_pubDate(time_t t);

		bool operator<(const rss_item& item) const {
			return item.pubDate_ < this->pubDate_;    // new items come first
		}

		inline const std::string& guid() const {
			return guid_;
		}
		void set_guid(const std::string& g);

		inline bool unread() const {
			return unread_;
		}
		void set_unread(bool u);
		void set_unread_nowrite(bool u);
		void set_unread_nowrite_notify(bool u, bool notify);

		inline void set_cache(cache * c) {
			ch = c;
		}
		inline void set_feedurl(const std::string& f) {
			feedurl_ = f;
		}

		inline const std::string& feedurl() const {
			return feedurl_;
		}

		inline const std::string& enclosure_url() const {
			return enclosure_url_;
		}
		inline const std::string& enclosure_type() const {
			return enclosure_type_;
		}

		void set_enclosure_url(const std::string& url);
		void set_enclosure_type(const std::string& type);

		inline bool enqueued() {
			return enqueued_;
		}
		inline void set_enqueued(bool v) {
			enqueued_ = v;
		}

		inline const std::string& flags() const {
			return flags_;
		}
		inline const std::string& oldflags() const {
			return oldflags_;
		}
		void set_flags(const std::string& ff);
		void update_flags();
		void sort_flags();

		virtual bool has_attribute(const std::string& attribname);
		virtual std::string get_attribute(const std::string& attribname);

		void set_feedptr(std::shared_ptr<rss_feed> ptr);
		inline std::shared_ptr<rss_feed> get_feedptr() {
			return feedptr_.lock();
		}

		inline bool deleted() const {
			return deleted_;
		}
		inline void set_deleted(bool b) {
			deleted_ = b;
		}

		inline void set_index(unsigned int i) {
			idx = i;
		}
		inline unsigned int get_index() {
			return idx;
		}

		inline void set_base(const std::string& b) {
			base = b;
		}
		inline const std::string& get_base() {
			return base;
		}

		inline void set_override_unread(bool b) {
			override_unread_ = b;
		}
		inline bool override_unread() {
			return override_unread_;
		}

		inline void unload() {
			description_.clear();
		}

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
		std::string oldflags_;
		std::weak_ptr<rss_feed> feedptr_;
		bool deleted_;
		unsigned int idx;
		std::string base;
		bool override_unread_;
		unsigned int size_;
};

class rss_feed : public matchable {
	public:
		explicit rss_feed(cache * c);
		rss_feed();
		~rss_feed();
		std::string title_raw() const {
			return title_;
		}
		std::string title() const;
		inline void set_title(const std::string& t) {
			title_ = t;
			utils::trim(title_);
		}

		std::string description_raw() const {
			return description_;
		}
		std::string description() const;
		inline void set_description(const std::string& d) {
			description_ = d;
		}

		inline const std::string& link() const {
			return link_;
		}
		inline void set_link(const std::string& l) {
			link_ = l;
		}

		inline std::string pubDate() const {
			return "TODO";
		}
		inline void set_pubDate(time_t t) {
			pubDate_ = t;
		}

		bool hidden() const;

		inline std::vector<std::shared_ptr<rss_item>>& items() {
			return items_;
		}
		inline void add_item(std::shared_ptr<rss_item> item) {
			items_.push_back(item);
			items_guid_map[item->guid()] = item;
		}
		inline void add_items(const std::vector<std::shared_ptr<rss_item>>& items) {
			for (const auto& item : items) {
				items_.push_back(item);
				items_guid_map[item->guid()] = item;
			}
		}
		inline void set_items(std::vector<std::shared_ptr<rss_item>>& items) {
			erase_items(items_.begin(), items_.end());
			add_items(items);
		}

		inline void clear_items() {
			LOG(level::DEBUG, "rss_feed: clearing items");
			items_.clear();
			items_guid_map.clear();
		}

		inline void erase_items(std::vector<std::shared_ptr<rss_item>>::iterator begin, std::vector<std::shared_ptr<rss_item>>::iterator end) {
			for (auto it=begin; it!=end; ++it) {
				items_guid_map.erase((*it)->guid());
			}
			items_.erase(begin, end);
		}
		inline void erase_item(std::vector<std::shared_ptr<rss_item>>::iterator pos) {
			items_guid_map.erase((*pos)->guid());
			items_.erase(pos);
		}

		std::shared_ptr<rss_item> get_item_by_guid(const std::string& guid);
		std::shared_ptr<rss_item> get_item_by_guid_unlocked(const std::string& guid);

		inline const std::string& rssurl() const {
			return rssurl_;
		}
		void set_rssurl(const std::string& u);

		unsigned int unread_item_count();
		inline unsigned int total_item_count() const {
			return items_.size();
		}

		void set_tags(const std::vector<std::string>& tags);
		bool matches_tag(const std::string& tag);
		std::string get_tags();
		std::string get_firsttag();

		virtual bool has_attribute(const std::string& attribname);
		virtual std::string get_attribute(const std::string& attribname);

		void update_items(std::vector<std::shared_ptr<rss_feed>> feeds);

		inline void set_query(const std::string& s) {
			query = s;
		}

		bool is_empty() {
			return empty;
		}
		void set_empty(bool t) {
			empty = t;
		}

		void sort(const std::string& method);
		void sort_unlocked(const std::string& method);

		void remove_old_deleted_items();

		void purge_deleted_items();

		inline void set_rtl(bool b) {
			is_rtl_ = b;
		}
		inline bool is_rtl() {
			return is_rtl_;
		}

		inline void set_index(unsigned int i) {
			idx = i;
		}
		inline unsigned int get_index() {
			return idx;
		}

		inline void set_order(unsigned int x) {
			order = x;
		}
		inline unsigned int get_order() {
			return order;
		}

		void set_feedptrs(std::shared_ptr<rss_feed> self);

		std::string get_status();

		inline void reset_status() {
			status_ = dl_status::TO_BE_DOWNLOADED;
		}
		inline void set_status(dl_status st) {
			status_ = st;
		}

		void unload();
		void load();

		std::mutex item_mutex; // this is ugly, but makes it possible to lock items use e.g. from the cache class
	private:
		std::string title_;
		std::string description_;
		std::string link_;
		time_t pubDate_;
		std::string rssurl_;
		std::vector<std::shared_ptr<rss_item>> items_;
		std::unordered_map<std::string, std::shared_ptr<rss_item>> items_guid_map;
		std::vector<std::string> tags_;
		std::string query;

		cache * ch;

		bool empty;
		bool is_rtl_;
		unsigned int idx;
		unsigned int order;
		dl_status status_;
		std::mutex items_guid_map_mutex;
};

class rss_ignores : public config_action_handler {
	public:
		rss_ignores() { }
		virtual ~rss_ignores();
		virtual void handle_action(const std::string& action, const std::vector<std::string>& params);
		virtual void dump_config(std::vector<std::string>& config_output);
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

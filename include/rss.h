#ifndef NEWSBOAT_RSS_H_
#define NEWSBOAT_RSS_H_

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "configcontainer.h"
#include "matcher.h"
#include "utils.h"

namespace newsboat {

typedef std::pair<std::string, matcher*> feedurl_expr_pair;

enum class dl_status { SUCCESS, TO_BE_DOWNLOADED, DURING_DOWNLOAD, DL_ERROR };

class cache;
class rss_feed;

class rss_item : public matchable {
public:
	explicit rss_item(cache* c);
	~rss_item() override;

	std::string title() const;
	std::string title_raw() const
	{
		return title_;
	}
	void set_title(const std::string& t);

	const std::string& link() const
	{
		return link_;
	}
	void set_link(const std::string& l);

	std::string author() const;
	std::string author_raw() const
	{
		return author_;
	}
	void set_author(const std::string& a);

	std::string description() const;
	std::string description_raw() const
	{
		return description_;
	}
	void set_description(const std::string& d);

	unsigned int size() const
	{
		return size_;
	}
	void set_size(unsigned int size);

	std::string length() const;
	std::string pubDate() const;

	time_t pubDate_timestamp() const
	{
		return pubDate_;
	}
	void set_pubDate(time_t t);

	bool operator<(const rss_item& item) const
	{
		return item.pubDate_ < this->pubDate_; // new items come first
	}

	const std::string& guid() const
	{
		return guid_;
	}
	void set_guid(const std::string& g);

	bool unread() const
	{
		return unread_;
	}
	void set_unread(bool u);
	void set_unread_nowrite(bool u);
	void set_unread_nowrite_notify(bool u, bool notify);

	void set_cache(cache* c)
	{
		ch = c;
	}
	void set_feedurl(const std::string& f)
	{
		feedurl_ = f;
	}

	const std::string& feedurl() const
	{
		return feedurl_;
	}

	const std::string& enclosure_url() const
	{
		return enclosure_url_;
	}
	const std::string& enclosure_type() const
	{
		return enclosure_type_;
	}

	void set_enclosure_url(const std::string& url);
	void set_enclosure_type(const std::string& type);

	bool enqueued()
	{
		return enqueued_;
	}
	void set_enqueued(bool v)
	{
		enqueued_ = v;
	}

	const std::string& flags() const
	{
		return flags_;
	}
	const std::string& oldflags() const
	{
		return oldflags_;
	}
	void set_flags(const std::string& ff);
	void update_flags();
	void sort_flags();

	bool has_attribute(const std::string& attribname) override;
	std::string get_attribute(const std::string& attribname) override;

	void set_feedptr(std::shared_ptr<rss_feed> ptr);
	std::shared_ptr<rss_feed> get_feedptr()
	{
		return feedptr_.lock();
	}

	bool deleted() const
	{
		return deleted_;
	}
	void set_deleted(bool b)
	{
		deleted_ = b;
	}

	void set_index(unsigned int i)
	{
		idx = i;
	}
	unsigned int get_index()
	{
		return idx;
	}

	void set_base(const std::string& b)
	{
		base = b;
	}
	const std::string& get_base()
	{
		return base;
	}

	void set_override_unread(bool b)
	{
		override_unread_ = b;
	}
	bool override_unread()
	{
		return override_unread_;
	}

	void unload()
	{
		description_.clear();
	}

private:
	std::string title_;
	std::string link_;
	std::string author_;
	std::string description_;
	std::string guid_;
	std::string feedurl_;
	cache* ch;
	std::string enclosure_url_;
	std::string enclosure_type_;
	std::string flags_;
	std::string oldflags_;
	std::weak_ptr<rss_feed> feedptr_;
	std::string base;
	unsigned int idx;
	unsigned int size_;
	time_t pubDate_;
	bool unread_;
	bool enqueued_;
	bool deleted_;
	bool override_unread_;
};

class rss_feed : public matchable {
public:
	explicit rss_feed(cache* c);
	rss_feed();
	~rss_feed() override;
	std::string title_raw() const
	{
		return title_;
	}
	std::string title() const;
	void set_title(const std::string& t)
	{
		title_ = t;
		utils::trim(title_);
	}

	std::string description_raw() const
	{
		return description_;
	}
	std::string description() const;
	void set_description(const std::string& d)
	{
		description_ = d;
	}

	const std::string& link() const
	{
		return link_;
	}
	void set_link(const std::string& l)
	{
		link_ = l;
	}

	std::string pubDate() const
	{
		return "TODO";
	}
	void set_pubDate(time_t t)
	{
		pubDate_ = t;
	}

	bool hidden() const;

	std::vector<std::shared_ptr<rss_item>>& items()
	{
		return items_;
	}
	void add_item(std::shared_ptr<rss_item> item)
	{
		items_.push_back(item);
		items_guid_map[item->guid()] = item;
	}
	void add_items(const std::vector<std::shared_ptr<rss_item>>& items)
	{
		for (const auto& item : items) {
			items_.push_back(item);
			items_guid_map[item->guid()] = item;
		}
	}
	void set_items(std::vector<std::shared_ptr<rss_item>>& items)
	{
		erase_items(items_.begin(), items_.end());
		add_items(items);
	}

	void clear_items()
	{
		LOG(level::DEBUG, "rss_feed: clearing items");
		items_.clear();
		items_guid_map.clear();
	}

	void erase_items(
		std::vector<std::shared_ptr<rss_item>>::iterator begin,
		std::vector<std::shared_ptr<rss_item>>::iterator end)
	{
		for (auto it = begin; it != end; ++it) {
			items_guid_map.erase((*it)->guid());
		}
		items_.erase(begin, end);
	}
	void erase_item(std::vector<std::shared_ptr<rss_item>>::iterator pos)
	{
		items_guid_map.erase((*pos)->guid());
		items_.erase(pos);
	}

	std::shared_ptr<rss_item> get_item_by_guid(const std::string& guid);
	std::shared_ptr<rss_item>
	get_item_by_guid_unlocked(const std::string& guid);

	const std::string& rssurl() const
	{
		return rssurl_;
	}
	void set_rssurl(const std::string& u);

	unsigned int unread_item_count();
	unsigned int total_item_count() const
	{
		return items_.size();
	}

	void set_tags(const std::vector<std::string>& tags);
	bool matches_tag(const std::string& tag);
	std::string get_tags();
	std::string get_firsttag();

	bool has_attribute(const std::string& attribname) override;
	std::string get_attribute(const std::string& attribname) override;

	void update_items(std::vector<std::shared_ptr<rss_feed>> feeds);

	void set_query(const std::string& s)
	{
		query = s;
	}

	bool is_empty()
	{
		return empty;
	}
	void set_empty(bool t)
	{
		empty = t;
	}

	void sort(const std::string& method);
	void sort_unlocked(const std::string& method);

	void remove_old_deleted_items();

	void purge_deleted_items();

	void set_rtl(bool b)
	{
		is_rtl_ = b;
	}
	bool is_rtl()
	{
		return is_rtl_;
	}

	void set_index(unsigned int i)
	{
		idx = i;
	}
	unsigned int get_index()
	{
		return idx;
	}

	void set_order(unsigned int x)
	{
		order = x;
	}
	unsigned int get_order()
	{
		return order;
	}

	void set_feedptrs(std::shared_ptr<rss_feed> self);

	std::string get_status();

	void reset_status()
	{
		status_ = dl_status::TO_BE_DOWNLOADED;
	}
	void set_status(dl_status st)
	{
		status_ = st;
	}

	void unload();
	void load();

	std::mutex item_mutex; // this is ugly, but makes it possible to lock
			       // items use e.g. from the cache class
private:
	std::string title_;
	std::string description_;
	std::string link_;
	time_t pubDate_;
	std::string rssurl_;
	std::vector<std::shared_ptr<rss_item>> items_;
	std::unordered_map<std::string, std::shared_ptr<rss_item>>
		items_guid_map;
	std::vector<std::string> tags_;
	std::string query;

	cache* ch;

	bool empty;
	bool is_rtl_;
	unsigned int idx;
	unsigned int order;
	dl_status status_;
	std::mutex items_guid_map_mutex;
};

class rss_ignores : public config_action_handler {
public:
	rss_ignores() {}
	~rss_ignores() override;
	void handle_action(
		const std::string& action,
		const std::vector<std::string>& params) override;
	void dump_config(std::vector<std::string>& config_output) override;
	bool matches(rss_item* item);
	bool matches_lastmodified(const std::string& url);
	bool matches_resetunread(const std::string& url);

private:
	std::vector<feedurl_expr_pair> ignores;
	std::vector<std::string> ignores_lastmodified;
	std::vector<std::string> resetflag;
};

} // namespace newsboat

#endif /* NEWSBOAT_RSS_H_ */

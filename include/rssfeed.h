#ifndef NEWSBOAT_RSSFEED_H_
#define NEWSBOAT_RSSFEED_H_

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "matchable.h"
#include "rssitem.h"
#include "utils.h"

namespace newsboat {

enum class DlStatus { SUCCESS, TO_BE_DOWNLOADED, DURING_DOWNLOAD, DL_ERROR };

class Cache;

class RssFeed : public Matchable {
public:
	explicit RssFeed(Cache* c);
	RssFeed();
	~RssFeed() override;
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

	std::vector<std::shared_ptr<RssItem>>& items()
	{
		return items_;
	}
	void add_item(std::shared_ptr<RssItem> item)
	{
		items_.push_back(item);
		items_guid_map[item->guid()] = item;
	}
	void add_items(const std::vector<std::shared_ptr<RssItem>>& items)
	{
		for (const auto& item : items) {
			items_.push_back(item);
			items_guid_map[item->guid()] = item;
		}
	}
	void set_items(std::vector<std::shared_ptr<RssItem>>& items)
	{
		erase_items(items_.begin(), items_.end());
		add_items(items);
	}

	void clear_items()
	{
		LOG(Level::DEBUG, "RssFeed: clearing items");
		items_.clear();
		items_guid_map.clear();
	}

	void erase_items(std::vector<std::shared_ptr<RssItem>>::iterator begin,
		std::vector<std::shared_ptr<RssItem>>::iterator end)
	{
		for (auto it = begin; it != end; ++it) {
			items_guid_map.erase((*it)->guid());
		}
		items_.erase(begin, end);
	}
	void erase_item(std::vector<std::shared_ptr<RssItem>>::iterator pos)
	{
		items_guid_map.erase((*pos)->guid());
		items_.erase(pos);
	}

	std::shared_ptr<RssItem> get_item_by_guid(const std::string& guid);
	std::shared_ptr<RssItem> get_item_by_guid_unlocked(
		const std::string& guid);

	/// \brief User-specified feed URL. Can't be empty, otherwise we
	/// wouldn't be able to fetch the feed.
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

	void update_items(std::vector<std::shared_ptr<RssFeed>> feeds);

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

	bool is_query_feed() const
	{
		return rssurl_.substr(0, 6) == "query:";
	}

	bool is_search_feed() const
	{
		return search_feed;
	}

	void set_search_feed(bool b)
	{
		search_feed = b;
	}

	void sort(const ArticleSortStrategy& sort_strategy);
	void sort_unlocked(const ArticleSortStrategy& sort_strategy);

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

	void set_feedptrs(std::shared_ptr<RssFeed> self);

	std::string get_status();

	void reset_status()
	{
		status_ = DlStatus::TO_BE_DOWNLOADED;
	}
	void set_status(DlStatus st)
	{
		status_ = st;
	}

	void unload();
	void load();

	void mark_all_items_read();

	std::mutex item_mutex; // this is ugly, but makes it possible to lock
			       // items use e.g. from the Cache class
private:
	std::string title_;
	std::string description_;
	std::string link_;
	time_t pubDate_;
	std::string rssurl_;
	std::vector<std::shared_ptr<RssItem>> items_;
	std::unordered_map<std::string, std::shared_ptr<RssItem>>
		items_guid_map;
	std::vector<std::string> tags_;
	std::string query;

	Cache* ch;

	bool empty;
	bool search_feed;
	bool is_rtl_;
	unsigned int idx;
	unsigned int order;
	DlStatus status_;
	std::mutex items_guid_map_mutex;
};

} // namespace newsboat

#endif /* NEWSBOAT_RSSFEED_H_ */

#ifndef NEWSBOAT_RSSFEED_H_
#define NEWSBOAT_RSSFEED_H_

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "config.h"
#include "matchable.h"
#include "rssitem.h"
#include "utils.h"

namespace Newsboat {

enum class DlStatus { SUCCESS, TO_BE_DOWNLOADED, DURING_DOWNLOAD, DL_ERROR };

class Cache;

class RssFeed : public Matchable {
public:
	explicit RssFeed(Cache* c, const std::string& rssurl);
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

	std::string description() const
	{
		return description_;
	}
	void set_description(const std::string& d)
	{
		description_ = d;
	}

	/// \brief Feed's canonical URL. Empty if feed was never fetched.
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
		return utils::mt_strf_localtime(_("%a, %d %b %Y %T %z"), pubDate_);
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

	/// \brief User-specified feed URL.
	const std::string& rssurl() const
	{
		return rssurl_;
	}

	unsigned int unread_item_count() const;
	unsigned int total_item_count() const
	{
		return items_.size();
	}

	void set_tags(const std::vector<std::string>& tags);
	bool matches_tag(const std::string& tag);
	std::vector<std::string> get_tags() const;
	std::string get_firsttag();

	std::optional<std::string> attribute_value(const std::string& attr) const
	override;

	void update_items(std::vector<std::shared_ptr<RssFeed>> feeds);

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
		std::lock_guard<std::mutex> guard(status_mutex_);
		status_ = DlStatus::TO_BE_DOWNLOADED;
	}
	void set_status(DlStatus st)
	{
		std::lock_guard<std::mutex> guard(status_mutex_);
		status_ = st;
	}

	void unload();
	void load();

	void mark_all_items_read();

	// this is ugly, but makes it possible to lock items use e.g. from the Cache class
	mutable std::mutex item_mutex;

private:
	std::string title_;
	std::string description_;
	std::string link_;
	time_t pubDate_;
	const std::string rssurl_;
	std::vector<std::shared_ptr<RssItem>> items_;
	std::unordered_map<std::string, std::shared_ptr<RssItem>>
		items_guid_map;
	std::vector<std::string> tags_;
	std::string query;

	Cache* ch;

	bool search_feed;
	bool is_rtl_;
	unsigned int idx;
	unsigned int order;
	std::mutex items_guid_map_mutex;

	DlStatus status_;
	std::mutex status_mutex_;
};

} // namespace Newsboat

#endif /* NEWSBOAT_RSSFEED_H_ */

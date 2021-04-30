#ifndef NEWSBOAT_RSSFEED_H_
#define NEWSBOAT_RSSFEED_H_

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "matchable.h"
#include "rssitem.h"
#include "utf8string.h"
#include "utils.h"

namespace newsboat {

enum class DlStatus { SUCCESS, TO_BE_DOWNLOADED, DURING_DOWNLOAD, DL_ERROR };

class Cache;

class RssFeed : public Matchable {
public:
	explicit RssFeed(Cache* c, const std::string& rssurl);
	~RssFeed() override;
	std::string title_raw() const
	{
		return title_.to_utf8();
	}
	std::string title() const;
	void set_title(const std::string& t)
	{
		std::string tmp(t);
		utils::trim(tmp);
		title_ = Utf8String::from_utf8(tmp);
	}

	std::string description() const
	{
		return description_.to_utf8();
	}
	void set_description(const std::string& d)
	{
		description_ = Utf8String::from_utf8(d);
	}

	const std::string& link() const
	{
		return link_.to_utf8();
	}
	void set_link(const std::string& l)
	{
		link_ = Utf8String::from_utf8(l);
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
		items_guid_map[Utf8String::from_utf8(item->guid())] = item;
	}
	void add_items(const std::vector<std::shared_ptr<RssItem>>& items)
	{
		for (const auto& item : items) {
			items_.push_back(item);
			items_guid_map[Utf8String::from_utf8(item->guid())] = item;
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
			items_guid_map.erase(Utf8String::from_utf8((*it)->guid()));
		}
		items_.erase(begin, end);
	}
	void erase_item(std::vector<std::shared_ptr<RssItem>>::iterator pos)
	{
		items_guid_map.erase(Utf8String::from_utf8((*pos)->guid()));
		items_.erase(pos);
	}

	std::shared_ptr<RssItem> get_item_by_guid(const std::string& guid);
	std::shared_ptr<RssItem> get_item_by_guid_unlocked(
		const std::string& guid);

	/// \brief User-specified feed URL.
	const std::string& rssurl() const
	{
		return rssurl_.to_utf8();
	}

	unsigned int unread_item_count() const;
	unsigned int total_item_count() const
	{
		return items_.size();
	}

	void set_tags(const std::vector<std::string>& tags);
	bool matches_tag(const std::string& tag);
	std::string get_tags() const;
	std::string get_firsttag();

	nonstd::optional<std::string> attribute_value(const std::string& attr) const
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
	Utf8String title_;
	Utf8String description_;
	Utf8String link_;
	time_t pubDate_;
	const Utf8String rssurl_;
	std::vector<std::shared_ptr<RssItem>> items_;
	std::unordered_map<Utf8String, std::shared_ptr<RssItem>> items_guid_map;
	std::vector<Utf8String> tags_;
	Utf8String query;

	Cache* ch;

	bool search_feed;
	bool is_rtl_;
	unsigned int idx;
	unsigned int order;
	std::mutex items_guid_map_mutex;

	DlStatus status_;
	std::mutex status_mutex_;
};

} // namespace newsboat

#endif /* NEWSBOAT_RSSFEED_H_ */

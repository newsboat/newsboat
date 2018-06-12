#include "feedcontainer.h"

#include <algorithm> // stable_sort
#include <strings.h> // strcasecmp

namespace newsboat {

void FeedContainer::sort_feeds(configcontainer* cfg)
{
	std::lock_guard<std::mutex> feedslock(feeds_mutex);
	const auto sortmethod_info =
		utils::tokenize(cfg->get_configvalue("feed-sort-order"), "-");
	std::string sortmethod = sortmethod_info[0];
	std::string direction = "desc";
	if (sortmethod_info.size() > 1)
		direction = sortmethod_info[1];
	if (sortmethod == "none") {
		std::stable_sort(feeds.begin(),
			feeds.end(),
			[](std::shared_ptr<rss_feed> a,
				std::shared_ptr<rss_feed> b) {
				return a->get_order() < b->get_order();
			});
	} else if (sortmethod == "firsttag") {
		std::stable_sort(feeds.begin(),
			feeds.end(),
			[](std::shared_ptr<rss_feed> a,
				std::shared_ptr<rss_feed> b) {
				if (a->get_firsttag().length() == 0 ||
					b->get_firsttag().length() == 0) {
					return a->get_firsttag().length() >
						b->get_firsttag().length();
				}
				return strcasecmp(a->get_firsttag().c_str(),
					       b->get_firsttag().c_str()) < 0;
			});
	} else if (sortmethod == "title") {
		std::stable_sort(feeds.begin(),
			feeds.end(),
			[](std::shared_ptr<rss_feed> a,
				std::shared_ptr<rss_feed> b) {
				return strcasecmp(a->title().c_str(),
					       b->title().c_str()) < 0;
			});
	} else if (sortmethod == "articlecount") {
		std::stable_sort(feeds.begin(),
			feeds.end(),
			[](std::shared_ptr<rss_feed> a,
				std::shared_ptr<rss_feed> b) {
				return a->total_item_count() <
					b->total_item_count();
			});
	} else if (sortmethod == "unreadarticlecount") {
		std::stable_sort(feeds.begin(),
			feeds.end(),
			[](std::shared_ptr<rss_feed> a,
				std::shared_ptr<rss_feed> b) {
				return a->unread_item_count() <
					b->unread_item_count();
			});
	} else if (sortmethod == "lastupdated") {
		std::stable_sort(feeds.begin(),
			feeds.end(),
			[](std::shared_ptr<rss_feed> a,
				std::shared_ptr<rss_feed> b) {
				if (a->items().size() == 0 ||
					b->items().size() == 0) {
					return a->items().size() >
						b->items().size();
				}
				auto cmp =
					[](std::shared_ptr<rss_item> a,
						std::shared_ptr<rss_item> b) {
						return *a < *b;
					};
				auto& a_item =
					*std::min_element(a->items().begin(),
						a->items().end(),
						cmp);
				auto& b_item =
					*std::min_element(b->items().begin(),
						b->items().end(),
						cmp);
				return cmp(a_item, b_item);
			});
	}
	if (direction == "asc") {
		std::reverse(feeds.begin(), feeds.end());
	}
}

std::shared_ptr<rss_feed> FeedContainer::get_feed(const unsigned int pos)
{
	std::lock_guard<std::mutex> feedslock(feeds_mutex);
	if (pos >= feeds.size()) {
		throw std::out_of_range(_("invalid feed index (bug)"));
	}
	std::shared_ptr<rss_feed> feed = feeds[pos];
	return feed;
}

void FeedContainer::mark_all_feed_items_read(const unsigned int feed_pos)
{
	const auto feed = get_feed(feed_pos);
	std::lock_guard<std::mutex> lock(feed->item_mutex);
	std::vector<std::shared_ptr<rss_item>>& items = feed->items();
	if (items.size() > 0) {
		bool notify = items[0]->feedurl() != feed->rssurl();
		LOG(level::DEBUG,
			"FeedContainer::mark_all_read: notify = %s",
			notify ? "yes" : "no");
		for (const auto& item : items) {
			item->set_unread_nowrite_notify(false, notify);
		}
	}
}

void FeedContainer::add_feed(const std::shared_ptr<rss_feed> feed)
{
	std::lock_guard<std::mutex> feedslock(feeds_mutex);
	feeds.push_back(feed);
}

void FeedContainer::populate_query_feeds()
{
	std::lock_guard<std::mutex> feedslock(feeds_mutex);
	for (const auto& feed : feeds) {
		if (feed->rssurl().substr(0, 6) == "query:") {
			feed->update_items(feeds);
		}
	}
}

unsigned int FeedContainer::get_feed_count_per_tag(const std::string& tag)
{
	unsigned int count = 0;
	std::lock_guard<std::mutex> feedslock(feeds_mutex);
	for (const auto& feed : feeds) {
		if (feed->matches_tag(tag)) {
			count++;
		}
	}

	return count;
}

std::shared_ptr<rss_feed> FeedContainer::get_feed_by_url(
	const std::string& feedurl)
{
	for (const auto& feed : feeds) {
		if (feedurl == feed->rssurl())
			return feed;
	}
	LOG(level::ERROR, "FeedContainer:get_feed_by_url failed for %s", feedurl);
	return std::shared_ptr<rss_feed>();
}

unsigned int FeedContainer::get_pos_of_next_unread(unsigned int pos)
{
	std::lock_guard<std::mutex> feedslock(feeds_mutex);
	for (pos++; pos < feeds.size(); pos++) {
		if (feeds[pos]->unread_item_count() > 0)
			break;
	}
	return pos;
}

unsigned int FeedContainer::feeds_size()
{
	std::lock_guard<std::mutex> feedslock(feeds_mutex);
	return feeds.size();
}

void FeedContainer::reset_feeds_status()
{
	std::lock_guard<std::mutex> feedlock(feeds_mutex);
	for (const auto& feed : feeds) {
		feed->reset_status();
	}
}

void FeedContainer::set_feeds(
	const std::vector<std::shared_ptr<rss_feed>> new_feeds)
{
	std::lock_guard<std::mutex> feedslock(feeds_mutex);
	feeds = new_feeds;
}

std::vector<std::shared_ptr<rss_feed>> FeedContainer::get_all_feeds()
{
	std::vector<std::shared_ptr<rss_feed>> tmpfeeds;
	{
		std::lock_guard<std::mutex> feedslock(feeds_mutex);
		tmpfeeds = feeds;
	}
	return tmpfeeds;
}

void FeedContainer::clear_feeds_items()
{
	std::lock_guard<std::mutex> feedslock(feeds_mutex);
	for (const auto& feed : feeds) {
		std::lock_guard<std::mutex> lock(feed->item_mutex);
		feed->clear_items();
	}
}

} // namespace newsboat

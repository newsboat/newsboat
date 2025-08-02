#include "feedcontainer.h"

#include <algorithm> // stable_sort
#include <numeric>   // accumulate
#include <unordered_set>

#include "logger.h"
#include "rssfeed.h"

namespace newsboat {

void FeedContainer::sort_feeds(const FeedSortStrategy& sort_strategy)
{
	std::lock_guard<std::mutex> feedslock(feeds_mutex);

	switch (sort_strategy.sm) {
	case FeedSortMethod::NONE:
		std::stable_sort(
			feeds.begin(), feeds.end(),
		[&](std::shared_ptr<RssFeed> a, std::shared_ptr<RssFeed> b) {
			if (sort_strategy.sd == SortDirection::ASC) {
				return a->get_order() > b->get_order();
			} else {
				return a->get_order() < b->get_order();
			}
		});
		break;
	case FeedSortMethod::FIRST_TAG:
		std::stable_sort(
			feeds.begin(), feeds.end(),
		[&](std::shared_ptr<RssFeed> a, std::shared_ptr<RssFeed> b) {
			if (a->get_firsttag().length() == 0 ||
				b->get_firsttag().length() == 0) {
				bool result =
					a->get_firsttag().length() > b->get_firsttag().length();
				if (sort_strategy.sd == SortDirection::ASC) {
					result = !result;
				}
				return result;
			}
			const auto left = a->get_firsttag();
			const auto right = b->get_firsttag();
			bool result = utils::strnaturalcmp(left, right) < 0;
			if (sort_strategy.sd == SortDirection::ASC) {
				result = !result;
			}
			return result;
		});
		break;
	case FeedSortMethod::TITLE:
		std::stable_sort(
			feeds.begin(), feeds.end(),
		[&](std::shared_ptr<RssFeed> a, std::shared_ptr<RssFeed> b) {
			const auto left = a->title();
			const auto right = b->title();
			if (sort_strategy.sd == SortDirection::ASC) {
				return utils::strnaturalcmp(left, right) > 0;
			} else {
				return utils::strnaturalcmp(left, right) < 0;
			}
		});
		break;
	case FeedSortMethod::ARTICLE_COUNT:
		std::stable_sort(
			feeds.begin(), feeds.end(),
		[&](std::shared_ptr<RssFeed> a, std::shared_ptr<RssFeed> b) {
			if (sort_strategy.sd == SortDirection::ASC) {
				return a->total_item_count() > b->total_item_count();
			} else {
				return a->total_item_count() < b->total_item_count();
			}
		});
		break;
	case FeedSortMethod::UNREAD_ARTICLE_COUNT:
		std::stable_sort(
			feeds.begin(), feeds.end(),
		[&](std::shared_ptr<RssFeed> a, std::shared_ptr<RssFeed> b) {
			if (sort_strategy.sd == SortDirection::DESC) {
				return a->unread_item_count() < b->unread_item_count();
			} else {
				return a->unread_item_count() > b->unread_item_count();
			}
		});
		break;
	case FeedSortMethod::LAST_UPDATED:
		std::stable_sort(
			feeds.begin(), feeds.end(),
		[&](std::shared_ptr<RssFeed> a, std::shared_ptr<RssFeed> b) {
			if (a->items().size() == 0 || b->items().size() == 0) {
				bool result = a->items().size() > b->items().size();
				if (sort_strategy.sd == SortDirection::ASC) {
					result = !result;
				}
				return result;
			}
			auto cmp = [](std::shared_ptr<RssItem> a,
			std::shared_ptr<RssItem> b) {
				return *a < *b;
			};
			auto& a_item = *std::min_element(a->items().begin(),
					a->items().end(), cmp);
			auto& b_item = *std::min_element(b->items().begin(),
					b->items().end(), cmp);

			if (sort_strategy.sd == SortDirection::DESC) {
				return *a_item < *b_item;
			} else {
				return *b_item < *a_item;
			}
		});
		break;
	case FeedSortMethod::LATEST_UNREAD:
		std::stable_sort(
			feeds.begin(), feeds.end(),
		[&](std::shared_ptr<RssFeed> a, std::shared_ptr<RssFeed> b) {
			if (a->unread_item_count() == 0 || b->unread_item_count() == 0) {
				bool result = a->unread_item_count() > b->unread_item_count();
				if (sort_strategy.sd == SortDirection::ASC) {
					result = !result;
				}
				return result;
			}
			std::vector<std::shared_ptr<RssItem>> a_unread;
			std::copy_if(a->items().begin(), a->items().end(),
				std::back_inserter(a_unread),
			[](const std::shared_ptr<RssItem>& item) {
				return item->unread();
			});
			std::vector<std::shared_ptr<RssItem>> b_unread;
			std::copy_if(b->items().begin(), b->items().end(),
				std::back_inserter(b_unread),
			[](const std::shared_ptr<RssItem>& item) {
				return item->unread();
			});
			auto cmp = [](std::shared_ptr<RssItem> a,
			std::shared_ptr<RssItem> b) {
				return *a < *b;
			};
			auto& a_item = *std::min_element(a_unread.begin(),
					a_unread.end(), cmp);
			auto& b_item = *std::min_element(b_unread.begin(),
					b_unread.end(), cmp);

			if (sort_strategy.sd == SortDirection::DESC) {
				return *a_item < *b_item;
			} else {
				return *b_item < *a_item;
			}
		});
		break;
	}
}

std::shared_ptr<RssFeed> FeedContainer::get_feed(const unsigned int pos)
{
	std::lock_guard<std::mutex> feedslock(feeds_mutex);
	if (pos < feeds.size()) {
		return feeds[pos];
	}
	return nullptr;
}

void FeedContainer::mark_all_feed_items_read(std::shared_ptr<RssFeed> feed)
{
	std::lock_guard<std::mutex> lock(feed->item_mutex);
	std::vector<std::shared_ptr<RssItem>>& items = feed->items();
	if (items.size() > 0) {
		bool notify = items[0]->feedurl() != feed->rssurl();
		LOG(Level::DEBUG,
			"FeedContainer::mark_all_read: notify = %s",
			notify ? "yes" : "no");
		for (const auto& item : items) {
			item->set_unread_nowrite_notify(false, notify);
		}
	}
}

void FeedContainer::mark_all_feeds_read()
{
	std::lock_guard<std::mutex> feedslock(feeds_mutex);
	for (const auto& feed : feeds) {
		feed->mark_all_items_read();
	}
}

void FeedContainer::add_feed(const std::shared_ptr<RssFeed> feed)
{
	std::lock_guard<std::mutex> feedslock(feeds_mutex);
	feeds.push_back(feed);
}

void FeedContainer::populate_query_feeds()
{
	std::lock_guard<std::mutex> feedslock(feeds_mutex);
	for (const auto& feed : feeds) {
		if (feed->is_query_feed()) {
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

unsigned int FeedContainer::get_unread_feed_count_per_tag(
	const std::string& tag)
{
	unsigned int count = 0;
	std::lock_guard<std::mutex> feedslock(feeds_mutex);
	for (const auto& feed : feeds) {
		if (feed->matches_tag(tag) && feed->unread_item_count() > 0) {
			count++;
		}
	}

	return count;
}

unsigned int FeedContainer::get_unread_item_count_per_tag(
	const std::string& tag)
{
	unsigned int count = 0;
	std::lock_guard<std::mutex> feedslock(feeds_mutex);
	for (const auto& feed : feeds) {
		if (feed->matches_tag(tag)) {
			count += feed->unread_item_count();
		}
	}

	return count;
}

std::shared_ptr<RssFeed> FeedContainer::get_feed_by_url(
	const std::string& feedurl)
{
	std::lock_guard<std::mutex> feedslock(feeds_mutex);
	for (const auto& feed : feeds) {
		if (feedurl == feed->rssurl()) {
			return feed;
		}
	}
	LOG(Level::ERROR,
		"FeedContainer:get_feed_by_url failed for %s",
		feedurl);
	return std::shared_ptr<RssFeed>();
}

unsigned int FeedContainer::get_pos_of_next_unread(unsigned int pos)
{
	std::lock_guard<std::mutex> feedslock(feeds_mutex);
	for (pos++; pos < feeds.size(); pos++) {
		if (feeds[pos]->unread_item_count() > 0) {
			break;
		}
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
	const std::vector<std::shared_ptr<RssFeed>>& new_feeds)
{
	std::lock_guard<std::mutex> feedslock(feeds_mutex);
	feeds = new_feeds;
}

std::vector<std::shared_ptr<RssFeed>> FeedContainer::get_all_feeds() const
{
	std::lock_guard<std::mutex> feedslock(feeds_mutex);
	return feeds;
}

unsigned int FeedContainer::unread_feed_count() const
{
	std::lock_guard<std::mutex> feedslock(feeds_mutex);
	return std::count_if(feeds.begin(),
			feeds.end(),
	[](const std::shared_ptr<RssFeed> feed) {
		return feed->unread_item_count() > 0;
	});
}

unsigned int FeedContainer::unread_item_count() const
{
	std::lock_guard<std::mutex> feedslock(feeds_mutex);

	using guid_set = std::unordered_set<std::string>;
	const auto unread_guids =
		std::accumulate(feeds.begin(),
			feeds.end(),
			guid_set(),
	[](guid_set guids, const std::shared_ptr<RssFeed> feed) {
		// Hidden feeds can't be viewed. The only way to read their articles is
		// via a query feed; items that aren't in query feeds are completely
		// inaccessible. Thus, we skip hidden feeds altogether to avoid
		// counting items that can't be accessed.
		if (feed->hidden()) {
			return guids;
		}

		std::lock_guard<std::mutex> itemslock(feed->item_mutex);
		for (const auto& item : feed->items()) {
			if (item->unread()) {
				guids.insert(item->guid());
			}
		}

		return guids;
	});

	return unread_guids.size();
}

void FeedContainer::replace_feed(unsigned int pos,
	std::shared_ptr<RssFeed> feed)
{
	std::lock_guard<std::mutex> feedslock(feeds_mutex);
	assert(pos < feeds.size());
	feeds[pos] = feed;
}

} // namespace newsboat

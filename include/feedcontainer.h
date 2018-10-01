#ifndef NEWSBOAT_FEEDCONTAINER_H_
#define NEWSBOAT_FEEDCONTAINER_H_

#include <memory>
#include <string>
#include <vector>

#include "rss.h"

namespace newsboat {
class FeedContainer {
public:
	FeedContainer() = default;

	void sort_feeds(const FeedSortStrategy& sort_strategy);
	std::shared_ptr<RssFeed> get_feed(const unsigned int pos);
	void add_feed(const std::shared_ptr<RssFeed> feed);
	void mark_all_feed_items_read(const unsigned int feed_pos);
	void mark_all_feeds_read();
	unsigned int get_feed_count_per_tag(const std::string& tag);
	std::shared_ptr<RssFeed> get_feed_by_url(const std::string& feedurl);
	void populate_query_feeds();
	unsigned int get_pos_of_next_unread(unsigned int pos);
	unsigned int feeds_size();
	void reset_feeds_status();
	void set_feeds(const std::vector<std::shared_ptr<RssFeed>> new_feeds);
	std::vector<std::shared_ptr<RssFeed>> get_all_feeds();
	void clear_feeds_items();
	unsigned int unread_feed_count() const;
	unsigned int unread_item_count() const;

	std::vector<std::shared_ptr<RssFeed>> feeds;

private:
	std::mutex feeds_mutex;
};
} // namespace newsboat

#endif /* NEWSBOAT_FEEDCONTAINER_H_ */

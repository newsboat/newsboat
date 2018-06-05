#include "feedhandler.h"

#include <algorithm> // stable_sort
#include <strings.h> // strcasecmp

namespace newsboat {

FeedHandler::FeedHandler()
{}

void FeedHandler::sort_feeds(const std::vector<std::string>& sortmethod_info)
{
	std::lock_guard<std::mutex> feedslock(feeds_mutex);
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

std::shared_ptr<rss_feed> FeedHandler::get_feed(const unsigned int pos)
{
	std::lock_guard<std::mutex> feedslock(feeds_mutex);
	if (pos >= feeds.size()) {
		throw std::out_of_range(_("invalid feed index (bug)"));
	}
	std::shared_ptr<rss_feed> feed = feeds[pos];
	return feed;
}

unsigned int FeedHandler::get_feed_count_per_tag(const std::string& tag)
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

std::shared_ptr<rss_feed> FeedHandler::get_feed_by_url(
	const std::string& feedurl)
{
	for (const auto& feed : feeds) {
		if (feedurl == feed->rssurl())
			return feed;
	}
	LOG(level::ERROR, "controller:get_feed_by_url failed for %s", feedurl);
	return std::shared_ptr<rss_feed>();
}

unsigned int FeedHandler::get_pos_of_next_unread(unsigned int pos)
{
	std::lock_guard<std::mutex> feedslock(feeds_mutex);
	for (pos++; pos < feeds.size(); pos++) {
		if (feeds[pos]->unread_item_count() > 0)
			break;
	}
	return pos;
}

} // namespace newsboat

#include "rssfeed.h"

#include <algorithm>
#include <cinttypes>
#include <cstring>
#include <curl/curl.h>
#include <langinfo.h>
#include <random>
#include <string.h>
#include <sys/utsname.h>
#include <time.h>

#include "cache.h"
#include "config.h"
#include "configcontainer.h"
#include "logger.h"
#include "scopemeasure.h"
#include "strprintf.h"
#include "utils.h"

namespace newsboat {

RssFeed::RssFeed(Cache* c, const std::string& rssurl)
	: pubDate_(0)
	, rssurl_(rssurl)
	, ch(c)
	, search_feed(false)
	, is_rtl_(false)
	, idx(0)
	, order(0)
	, status_(DlStatus::SUCCESS)
{
	RssFeedRegistry::get_instance()->register_rss_feed(this);

	if (utils::is_query_url(rssurl_)) {
		/* Query string looks like this:
		 *
		 * query:Title:unread = "yes" and age between 0:7
		 *
		 * So we split by colons to get title and the query itself. */
		const auto tokens = utils::tokenize(rssurl_, ":");

		if (tokens.size() < 3) {
			throw _s("too few arguments");
		}

		/* "Between" operator requires a range, which contains a colon.
		 * Since we've been tokenizing by colon, we might've
		 * inadertently split the query itself. Let's reconstruct it! */
		auto query = tokens[2];
		for (auto it = tokens.begin() + 3; it != tokens.end(); ++it) {
			query += ":";
			query += *it;
		}
		// Have to check if the result is a valid query, just in case
		Matcher m;
		if (!m.parse(query)) {
			throw strprintf::fmt(
				_("couldn't parse filter expression `%s': %s"),
				query, m.get_parse_error());
		}

		LOG(Level::DEBUG,
			"RssFeed constructor: query name = `%s' expr = `%s'",
			tokens[1],
			query);

		set_title(tokens[1]);
		this->query = query;
	}
}

unsigned int RssFeed::unread_item_count() const
{
	std::lock_guard<std::mutex> lock(item_mutex);
	return std::count_if(items_.begin(),
			items_.end(),
	[](const std::shared_ptr<RssItem>& item) {
		return item->unread();
	});
}

bool RssFeed::matches_tag(const std::string& tag)
{
	return std::find_if(
	tags_.begin(), tags_.end(), [&](const std::string& t) {
		return tag == t;
	}) != tags_.end();
}

std::string RssFeed::get_firsttag()
{
	for (const auto& t : tags_) {
		if (t.substr(0, 1) != "~") {
			return t;
		}
	}
	return "";
}

std::vector<std::string> RssFeed::get_tags() const
{
	std::vector<std::string> tags;
	for (const auto& t : tags_) {
		if (t.substr(0, 1) != "~" && t.substr(0, 1) != "!") {
			tags.push_back(t);
		}
	}
	return tags;
}

void RssFeed::set_tags(const std::vector<std::string>& tags)
{
	tags_ = tags;
}

std::string RssFeed::title() const
{
	bool found_title = false;
	std::string alt_title;
	for (const auto& tag : tags_) {
		if (tag.substr(0, 1) == "~") {
			found_title = true;
			alt_title = tag.substr(1, tag.length() - 1);
			break;
		}
	}
	return found_title
		? alt_title
		: utils::utf8_to_locale(title_);
}

bool RssFeed::hidden() const
{
	return std::any_of(tags_.begin(),
			tags_.end(),
	[](const std::string& tag) {
		return tag.substr(0, 1) == "!";
	});
}

std::shared_ptr<RssItem> RssFeed::get_item_by_guid(const std::string& guid)
{
	std::lock_guard<std::mutex> lock(item_mutex);
	return get_item_by_guid_unlocked(guid);
}

std::shared_ptr<RssItem> RssFeed::get_item_by_guid_unlocked(
	const std::string& guid)
{
	auto it = items_guid_map.find(guid);
	if (it != items_guid_map.end()) {
		return it->second;
	}
	LOG(Level::DEBUG,
		"RssFeed::get_item_by_guid_unlocked: hit dummy item!");
	LOG(Level::DEBUG,
		"RssFeed::get_item_by_guid_unlocked: items_guid_map.size = %" PRIu64,
		static_cast<uint64_t>(items_guid_map.size()));

	// should never happen!
	return std::shared_ptr<RssItem>(new RssItem(ch));
}

std::optional<std::string> RssFeed::attribute_value(const std::string&
	attribname) const
{
	if (attribname == "feedtitle") {
		return title();
	} else if (attribname == "description") {
		return utils::utf8_to_locale(description());
	} else if (attribname == "feedlink") {
		return link();
	} else if (attribname == "feeddate") {
		return pubDate();
	} else if (attribname == "rssurl") {
		return rssurl();
	} else if (attribname == "unread_count") {
		return std::to_string(unread_item_count());
	} else if (attribname == "total_count") {
		return std::to_string(items_.size());
	} else if (attribname == "tags") {
		std::string tags;
		for (const std::string& t : get_tags()) {
			tags.append(t);
			tags.append(" ");
		}
		return tags;
	} else if (attribname == "feedindex") {
		return std::to_string(idx);
	} else if (attribname == "latest_article_age") {
		using ItemType = std::shared_ptr<RssItem>;
		const auto latest_article_iterator = std::max_element(items_.begin(),
		items_.end(), [](const ItemType& a, const ItemType& b) {
			return a->pubDate_timestamp() < b->pubDate_timestamp();
		});
		if (latest_article_iterator != items_.end()) {
			const auto latest_article = *latest_article_iterator;
			const auto timestamp = latest_article->pubDate_timestamp();
			return std::to_string((time(nullptr) - timestamp) / 86400);
		}
		return "0";
	}
	return std::nullopt;
}

void RssFeed::update_items(std::vector<std::shared_ptr<RssFeed>> feeds)
{
	std::lock_guard<std::mutex> lock(item_mutex);
	if (query.empty()) {
		return;
	}

	LOG(Level::DEBUG, "RssFeed::update_items: query = `%s'", query);

	ScopeMeasure sm("RssFeed::update_items");

	Matcher m(query);

	items_.clear();
	items_guid_map.clear();

	for (const auto& feed : feeds) {
		if (feed->is_query_feed()) {
			// don't fetch items from other query feeds!
			continue;
		}
		for (const auto& item : feed->items()) {
			if (!item->deleted() && m.matches(item.get())) {
				LOG(Level::DEBUG, "RssFeed::update_items: Matcher matches!");
				item->set_feedptr(feed);
				items_.push_back(item);
				items_guid_map[item->guid()] = item;
			}
		}
	}

	sm.stopover("matching");

	std::sort(items_.begin(), items_.end());

	sm.stopover("sorting");
}

void RssFeed::sort(const ArticleSortStrategy& sort_strategy)
{
	std::lock_guard<std::mutex> lock(item_mutex);
	sort_unlocked(sort_strategy);
}

void RssFeed::sort_unlocked(const ArticleSortStrategy& sort_strategy)
{
	switch (sort_strategy.sm) {
	case ArtSortMethod::TITLE:
		std::stable_sort(items_.begin(),
			items_.end(),
			[&](const std::shared_ptr<RssItem>& a,
		const std::shared_ptr<RssItem>& b) {
			const auto left = utils::utf8_to_locale(a->title());
			const auto right = utils::utf8_to_locale(b->title());
			const auto cmp = utils::strnaturalcmp(left, right);
			return sort_strategy.sd == SortDirection::DESC ? (cmp > 0) : (cmp < 0);
		});
		break;
	case ArtSortMethod::FLAGS:
		std::stable_sort(items_.begin(),
			items_.end(),
			[&](const std::shared_ptr<RssItem>& a,
		const std::shared_ptr<RssItem>& b) {
			return sort_strategy.sd ==
				SortDirection::DESC
				? (strcmp(a->flags().c_str(),
						b->flags().c_str()) > 0)
				: (strcmp(a->flags().c_str(),
						b->flags().c_str()) < 0);
		});
		break;
	case ArtSortMethod::AUTHOR:
		std::stable_sort(items_.begin(),
			items_.end(),
			[&](const std::shared_ptr<RssItem>& a,
		const std::shared_ptr<RssItem>& b) {
			const auto author_a = utils::utf8_to_locale(a->author());
			const auto author_b = utils::utf8_to_locale(b->author());
			const auto cmp = strcmp(author_a.c_str(), author_b.c_str());
			return sort_strategy.sd == SortDirection::DESC ? (cmp > 0) : (cmp < 0);
		});
		break;
	case ArtSortMethod::LINK:
		std::stable_sort(items_.begin(),
			items_.end(),
			[&](const std::shared_ptr<RssItem>& a,
		const std::shared_ptr<RssItem>& b) {
			return sort_strategy.sd ==
				SortDirection::DESC
				? (strcmp(a->link().c_str(),
						b->link().c_str()) > 0)
				: (strcmp(a->link().c_str(),
						b->link().c_str()) < 0);
		});
		break;
	case ArtSortMethod::GUID:
		std::stable_sort(items_.begin(),
			items_.end(),
			[&](const std::shared_ptr<RssItem>& a,
		const std::shared_ptr<RssItem>& b) {
			return sort_strategy.sd ==
				SortDirection::DESC
				? (strcmp(a->guid().c_str(),
						b->guid().c_str()) > 0)
				: (strcmp(a->guid().c_str(),
						b->guid().c_str()) < 0);
		});
		break;
	case ArtSortMethod::DATE:
		std::stable_sort(items_.begin(),
			items_.end(),
			[&](const std::shared_ptr<RssItem>& a,
		const std::shared_ptr<RssItem>& b) {
			// date is descending by default
			return sort_strategy.sd == SortDirection::ASC
				? (a->pubDate_timestamp() >
					b->pubDate_timestamp())
				: (a->pubDate_timestamp() <
					b->pubDate_timestamp());
		});
		break;
	case ArtSortMethod::RANDOM:
		std::random_device rd;
		std::default_random_engine rng(rd());
		std::shuffle(items_.begin(), items_.end(), rng);
		break;
	}
}

void RssFeed::purge_deleted_items()
{
	std::lock_guard<std::mutex> lock(item_mutex);
	ScopeMeasure m1("RssFeed::purge_deleted_items");

	// Purge in items_guid_map
	{
		std::lock_guard<std::mutex> lock2(items_guid_map_mutex);
		for (const auto& item : items_) {
			if (item->deleted()) {
				items_guid_map.erase(item->guid());
			}
		}
	}

	items_.erase(std::remove_if(items_.begin(),
			items_.end(),
	[](const std::shared_ptr<RssItem> item) {
		return item->deleted();
	}),
	items_.end());
}

void RssFeed::set_feedptrs(std::shared_ptr<RssFeed> self)
{
	std::lock_guard<std::mutex> lock(item_mutex);
	for (const auto& item : items_) {
		item->set_feedptr(self);
	}
}

std::string RssFeed::get_status()
{
	std::lock_guard<std::mutex> guard(status_mutex_);

	switch (status_) {
	case DlStatus::SUCCESS:
		return " ";
	case DlStatus::TO_BE_DOWNLOADED:
		return "_";
	case DlStatus::DURING_DOWNLOAD:
		return ".";
	case DlStatus::DL_ERROR:
		return "x";
	}
	return "?";
}

void RssFeed::unload()
{
	std::lock_guard<std::mutex> lock(item_mutex);
	for (const auto& item : items_) {
		item->unload();
	}
}

void RssFeed::load()
{
	std::lock_guard<std::mutex> lock(item_mutex);
	ch->fetch_descriptions(this);
}

void RssFeed::mark_all_items_read()
{
	std::lock_guard<std::mutex> lock(item_mutex);
	for (const auto& item : items_) {
		item->set_unread_nowrite(false);
	}
}

} // namespace newsboat

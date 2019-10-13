#include "rssfeed.h"

#include <algorithm>
#include <cerrno>
#include <cinttypes>
#include <cstring>
#include <curl/curl.h>
#include <functional>
#include <iostream>
#include <langinfo.h>
#include <sstream>
#include <string.h>
#include <sys/utsname.h>
#include <time.h>

#include "cache.h"
#include "config.h"
#include "configcontainer.h"
#include "confighandlerexception.h"
#include "dbexception.h"
#include "htmlrenderer.h"
#include "logger.h"
#include "scopemeasure.h"
#include "strprintf.h"
#include "tagsouppullparser.h"
#include "utils.h"

namespace newsboat {

RssFeed::RssFeed(Cache* c)
	: pubDate_(0)
	, ch(c)
	, empty(true)
	, search_feed(false)
	, is_rtl_(false)
	, idx(0)
	, order(0)
	, status_(DlStatus::SUCCESS)
{}

RssFeed::~RssFeed()
{
	clear_items();
}

unsigned int RssFeed::unread_item_count()
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
	return tags_.empty() ? "" : tags_.front();
}

std::string RssFeed::get_tags()
{
	std::string tags;
	for (const auto& t : tags_) {
		if (t.substr(0, 1) != "~" && t.substr(0, 1) != "!") {
			tags.append(t);
			tags.append(" ");
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
		: utils::convert_text(title_, nl_langinfo(CODESET), "utf-8");
}

std::string RssFeed::description() const
{
	return utils::convert_text(description_, nl_langinfo(CODESET), "utf-8");
}

bool RssFeed::hidden() const
{
	return std::any_of(tags_.begin(),
		tags_.end(),
		[](const std::string& tag) { return tag.substr(0, 1) == "!"; });
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
		"RssFeed::get_item_by_guid_unlocked: items_guid_map.size = "
		"%" PRIu64,
		static_cast<uint64_t>(items_guid_map.size()));

	// should never happen!
	return std::shared_ptr<RssItem>(new RssItem(ch));
}

bool RssFeed::has_attribute(const std::string& attribname)
{
	if (attribname == "feedtitle" || attribname == "description" ||
		attribname == "feedlink" || attribname == "feeddate" ||
		attribname == "rssurl" || attribname == "unread_count" ||
		attribname == "total_count" || attribname == "tags" ||
		attribname == "feedindex")
		return true;
	return false;
}

std::string RssFeed::get_attribute(const std::string& attribname)
{
	if (attribname == "feedtitle")
		return title();
	else if (attribname == "description")
		return description();
	else if (attribname == "feedlink")
		return title();
	else if (attribname == "feeddate")
		return pubDate();
	else if (attribname == "rssurl")
		return rssurl();
	else if (attribname == "unread_count") {
		return std::to_string(unread_item_count());
	} else if (attribname == "total_count") {
		return std::to_string(items_.size());
	} else if (attribname == "tags") {
		return get_tags();
	} else if (attribname == "feedindex") {
		return std::to_string(idx);
	}
	return "";
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
				LOG(Level::DEBUG,
					"RssFeed::update_items: Matcher "
					"matches!");
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

void RssFeed::set_rssurl(const std::string& u)
{
	rssurl_ = u;
	if (utils::is_query_url(u)) {
		/* Query string looks like this:
		 *
		 * query:Title:unread = "yes" and age between 0:7
		 *
		 * So we split by colons to get title and the query itself. */
		std::vector<std::string> tokens =
			utils::tokenize_quoted(u, ":");

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
				_("`%s' is not a valid filter expression"),
				query);
		}

		LOG(Level::DEBUG,
			"RssFeed::set_rssurl: query name = `%s' expr = `%s'",
			tokens[1],
			query);

		set_title(tokens[1]);
		set_query(query);
	}
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
				return sort_strategy.sd == SortDirection::DESC
					? (utils::strnaturalcmp(
						   a->title().c_str(),
						   b->title().c_str()) > 0)
					: (utils::strnaturalcmp(
						   a->title().c_str(),
						   b->title().c_str()) < 0);
			});
		break;
	case ArtSortMethod::FLAGS:
		std::stable_sort(items_.begin(),
			items_.end(),
			[&](const std::shared_ptr<RssItem>& a,
				const std::shared_ptr<RssItem>& b) {
				return sort_strategy.sd == SortDirection::DESC
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
				return sort_strategy.sd == SortDirection::DESC
					? (strcmp(a->author().c_str(),
						   b->author().c_str()) > 0)
					: (strcmp(a->author().c_str(),
						   b->author().c_str()) < 0);
			});
		break;
	case ArtSortMethod::LINK:
		std::stable_sort(items_.begin(),
			items_.end(),
			[&](const std::shared_ptr<RssItem>& a,
				const std::shared_ptr<RssItem>& b) {
				return sort_strategy.sd == SortDirection::DESC
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
				return sort_strategy.sd == SortDirection::DESC
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
		std::random_shuffle(items_.begin(), items_.end());
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

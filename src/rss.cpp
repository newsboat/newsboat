#include "rss.h"

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <curl/curl.h>
#include <functional>
#include <iostream>
#include <langinfo.h>
#include <sstream>
#include <sys/utsname.h>
#include <string.h>
#include <time.h>

#include "cache.h"
#include "config.h"
#include "configcontainer.h"
#include "exceptions.h"
#include "htmlrenderer.h"
#include "logger.h"
#include "strprintf.h"
#include "tagsouppullparser.h"
#include "utils.h"

namespace newsboat {

RssItem::RssItem(Cache* c)
	: ch(c)
	, idx(0)
	, size_(0)
	, pubDate_(0)
	, unread_(true)
	, enqueued_(false)
	, deleted_(0)
	, override_unread_(false)
{
}

RssItem::~RssItem() {}

RssFeed::RssFeed(Cache* c)
	: pubDate_(0)
	, ch(c)
	, empty(true)
	, is_rtl_(false)
	, idx(0)
	, order(0)
	, status_(DlStatus::SUCCESS)
{
}

RssFeed::~RssFeed()
{
	clear_items();
}

// RssItem setters

void RssItem::set_title(const std::string& t)
{
	title_ = t;
	utils::trim(title_);
}

void RssItem::set_link(const std::string& l)
{
	link_ = l;
	utils::trim(link_);
}

void RssItem::set_author(const std::string& a)
{
	author_ = a;
}

void RssItem::set_description(const std::string& d)
{
	description_ = d;
}

void RssItem::set_size(unsigned int size)
{
	size_ = size;
}

std::string RssItem::length() const
{
	std::string::size_type l(size_);
	if (!l)
		return "";
	if (l < 1000)
		return strprintf::fmt("%u ", l);
	if (l < 1024 * 1000)
		return strprintf::fmt("%.1fK", l / 1024.0);

	return strprintf::fmt("%.1fM", l / 1024.0 / 1024.0);
}

void RssItem::set_pubDate(time_t t)
{
	pubDate_ = t;
}

void RssItem::set_guid(const std::string& g)
{
	guid_ = g;
}

void RssItem::set_unread_nowrite(bool u)
{
	unread_ = u;
}

void RssItem::set_unread_nowrite_notify(bool u, bool notify)
{
	unread_ = u;
	std::shared_ptr<RssFeed> feedptr = feedptr_.lock();
	if (feedptr && notify) {
		feedptr->get_item_by_guid(guid_)->set_unread_nowrite(
			unread_); // notify parent feed
	}
}

void RssItem::set_unread(bool u)
{
	if (unread_ != u) {
		bool old_u = unread_;
		unread_ = u;
		std::shared_ptr<RssFeed> feedptr = feedptr_.lock();
		if (feedptr)
			feedptr->get_item_by_guid(guid_)->set_unread_nowrite(
				unread_); // notify parent feed
		try {
			if (ch) {
				ch->update_rssitem_unread_and_enqueued(
					this, feedurl_);
			}
		} catch (const DbException& e) {
			// if the update failed, restore the old unread flag and
			// rethrow the exception
			unread_ = old_u;
			throw;
		}
	}
}

std::string RssItem::pubDate() const
{
	char text[1024];
	strftime(text,
		sizeof(text),
		_("%a, %d %b %Y %T %z"),
		localtime(&pubDate_));
	return std::string(text);
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

void RssItem::set_enclosure_url(const std::string& url)
{
	enclosure_url_ = url;
}

void RssItem::set_enclosure_type(const std::string& type)
{
	enclosure_type_ = type;
}

std::string RssItem::title() const
{
	std::string retval;
	if (title_.length() > 0)
		retval = utils::convert_text(
			title_, nl_langinfo(CODESET), "utf-8");
	return retval;
}

std::string RssItem::author() const
{
	return utils::convert_text(author_, nl_langinfo(CODESET), "utf-8");
}

std::string RssItem::description() const
{
	return utils::convert_text(description_, nl_langinfo(CODESET), "utf-8");
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
		"RssFeed::get_item_by_guid_unlocked: items_guid_map.size = %d",
		items_guid_map.size());
	// abort();
	return std::shared_ptr<RssItem>(
		new RssItem(ch)); // should never happen!
}

bool RssItem::has_attribute(const std::string& attribname)
{
	if (attribname == "title" || attribname == "link" ||
		attribname == "author" || attribname == "content" ||
		attribname == "date" || attribname == "guid" ||
		attribname == "unread" || attribname == "enclosure_url" ||
		attribname == "enclosure_type" || attribname == "flags" ||
		attribname == "age" || attribname == "articleindex")
		return true;

	// if we have a feed, then forward the request
	std::shared_ptr<RssFeed> feedptr = feedptr_.lock();
	if (feedptr)
		return feedptr->RssFeed::has_attribute(attribname);

	return false;
}

std::string RssItem::get_attribute(const std::string& attribname)
{
	if (attribname == "title")
		return title();
	else if (attribname == "link")
		return link();
	else if (attribname == "author")
		return author();
	else if (attribname == "content")
		return description();
	else if (attribname == "date")
		return pubDate();
	else if (attribname == "guid")
		return guid();
	else if (attribname == "unread")
		return unread_ ? "yes" : "no";
	else if (attribname == "enclosure_url")
		return enclosure_url();
	else if (attribname == "enclosure_type")
		return enclosure_type();
	else if (attribname == "flags")
		return flags();
	else if (attribname == "age")
		return std::to_string(
			(time(nullptr) - pubDate_timestamp()) / 86400);
	else if (attribname == "articleindex")
		return std::to_string(idx);

	// if we have a feed, then forward the request
	std::shared_ptr<RssFeed> feedptr = feedptr_.lock();
	if (feedptr)
		return feedptr->RssFeed::get_attribute(attribname);

	return "";
}

void RssItem::update_flags()
{
	if (ch) {
		ch->update_rssitem_flags(this);
	}
}

void RssItem::set_flags(const std::string& ff)
{
	oldflags_ = flags_;
	flags_ = ff;
	sort_flags();
}

void RssItem::sort_flags()
{
	std::sort(flags_.begin(), flags_.end());

	// Erase non-alpha characters
	flags_.erase(std::remove_if(flags_.begin(),
			     flags_.end(),
			     [](const char c) { return !isalpha(c); }),
		flags_.end());

	// Erase doubled characters
	flags_.erase(std::unique(flags_.begin(), flags_.end()), flags_.end());
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

void RssIgnores::handle_action(const std::string& action,
	const std::vector<std::string>& params)
{
	if (action == "ignore-article") {
		if (params.size() < 2)
			throw ConfigHandlerException(
				ActionHandlerStatus::TOO_FEW_PARAMS);
		std::string ignore_rssurl = params[0];
		std::string ignore_expr = params[1];
		Matcher m;
		if (!m.parse(ignore_expr))
			throw ConfigHandlerException(strprintf::fmt(
				_("couldn't parse filter expression `%s': %s"),
				ignore_expr,
				m.get_parse_error()));
		ignores.push_back(FeedUrlExprPair(
			ignore_rssurl, new Matcher(ignore_expr)));
	} else if (action == "always-download") {
		for (const auto& param : params) {
			ignores_lastmodified.push_back(param);
		}
	} else if (action == "reset-unread-on-update") {
		for (const auto& param : params) {
			resetflag.push_back(param);
		}
	} else
		throw ConfigHandlerException(
			ActionHandlerStatus::INVALID_COMMAND);
}

void RssIgnores::dump_config(std::vector<std::string>& config_output)
{
	for (const auto& ign : ignores) {
		std::string configline = "ignore-article ";
		if (ign.first == "*")
			configline.append("*");
		else
			configline.append(utils::quote(ign.first));
		configline.append(" ");
		configline.append(utils::quote(ign.second->get_expression()));
		config_output.push_back(configline);
	}
	for (const auto& ign_lm : ignores_lastmodified) {
		config_output.push_back(strprintf::fmt(
			"always-download %s", utils::quote(ign_lm)));
	}
	for (const auto& rf : resetflag) {
		config_output.push_back(strprintf::fmt(
			"reset-unread-on-update %s", utils::quote(rf)));
	}
}

RssIgnores::~RssIgnores()
{
	for (const auto& ign : ignores) {
		delete ign.second;
	}
}

bool RssIgnores::matches(RssItem* item)
{
	for (const auto& ign : ignores) {
		LOG(Level::DEBUG,
			"RssIgnores::matches: ign.first = `%s' item->feedurl "
			"= "
			"`%s'",
			ign.first,
			item->feedurl());
		if (ign.first == "*" || item->feedurl() == ign.first) {
			if (ign.second->matches(item)) {
				LOG(Level::DEBUG,
					"RssIgnores::matches: found match");
				return true;
			}
		}
	}
	return false;
}

bool RssIgnores::matches_lastmodified(const std::string& url)
{
	return std::find_if(ignores_lastmodified.begin(),
		       ignores_lastmodified.end(),
		       [&](const std::string& u) { return u == url; }) !=
		ignores_lastmodified.end();
}

bool RssIgnores::matches_resetunread(const std::string& url)
{
	return std::find_if(resetflag.begin(),
		       resetflag.end(),
		       [&](const std::string& u) { return u == url; }) !=
		resetflag.end();
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
			[&](std::shared_ptr<RssItem> a,
				std::shared_ptr<RssItem> b) {
				return sort_strategy.sd ==
						SortDirection::DESC
					? (utils::strnaturalcmp(a->title().c_str(),
						   b->title().c_str()) > 0)
					: (utils::strnaturalcmp(a->title().c_str(),
						   b->title().c_str()) < 0);
			});
		break;
	case ArtSortMethod::FLAGS:
		std::stable_sort(items_.begin(),
			items_.end(),
			[&](std::shared_ptr<RssItem> a,
				std::shared_ptr<RssItem> b) {
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
			[&](std::shared_ptr<RssItem> a,
				std::shared_ptr<RssItem> b) {
				return sort_strategy.sd ==
						SortDirection::DESC
					? (strcmp(a->author().c_str(),
						   b->author().c_str()) > 0)
					: (strcmp(a->author().c_str(),
						   b->author().c_str()) < 0);
			});
		break;
	case ArtSortMethod::LINK:
		std::stable_sort(items_.begin(),
			items_.end(),
			[&](std::shared_ptr<RssItem> a,
				std::shared_ptr<RssItem> b) {
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
			[&](std::shared_ptr<RssItem> a,
				std::shared_ptr<RssItem> b) {
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
			[&](std::shared_ptr<RssItem> a,
				std::shared_ptr<RssItem> b) {
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

void RssFeed::remove_old_deleted_items()
{
	std::lock_guard<std::mutex> lock(item_mutex);
	std::vector<std::string> guids;
	for (const auto& item : items_) {
		guids.push_back(item->guid());
	}
	ch->remove_old_deleted_items(rssurl_, guids);
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

void RssItem::set_feedptr(std::shared_ptr<RssFeed> ptr)
{
	feedptr_ = std::weak_ptr<RssFeed>(ptr);
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

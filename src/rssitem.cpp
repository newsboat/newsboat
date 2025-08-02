#include "rssitem.h"

#include <algorithm>
#include <cinttypes>
#include <langinfo.h>

#include "cache.h"
#include "dbexception.h"
#include "rssfeed.h"
#include "scopemeasure.h"
#include "strprintf.h"
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

// RssItem setters

void RssItem::set_title(const std::string& t)
{
	title_ = utils::consolidate_whitespace(t);
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

void RssItem::set_description(const std::string& content,
	const std::string& mime_type)
{
	std::lock_guard<std::mutex> guard(description_mutex);
	description_ = {content, mime_type};
}

void RssItem::set_size(unsigned int size)
{
	size_ = size;
}

std::string RssItem::length() const
{
	std::string::size_type l(size_);
	if (!l) {
		return "";
	}
	if (l < 1000) {
		return strprintf::fmt("%" PRIu64 " ", static_cast<uint64_t>(l));
	}
	if (l < 1024 * 1000) {
		return strprintf::fmt("%.1fK", l / 1024.0);
	}

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
					*this, feedurl_);
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
	return utils::mt_strf_localtime(_("%a, %d %b %Y %T %z"), pubDate_);
}

const std::string& RssItem::feedurl() const
{
	return feedurl_;
}

const std::string& RssItem::enclosure_url() const
{
	return enclosure_url_;
}

const std::string& RssItem::enclosure_type() const
{
	return enclosure_type_;
}

const std::string& RssItem::enclosure_description() const
{
	return enclosure_description_;
}

const std::string& RssItem::enclosure_description_mime_type() const
{
	return enclosure_description_mime_type_;
}

void RssItem::set_enclosure_url(const std::string& url)
{
	enclosure_url_ = url;
}

void RssItem::set_enclosure_type(const std::string& type)
{
	enclosure_type_ = type;
}

void RssItem::set_enclosure_description(const std::string& description)
{
	enclosure_description_ = description;
}

void RssItem::set_enclosure_description_mime_type(const std::string& type)
{
	enclosure_description_mime_type_ = type;
}

std::optional<std::string> RssItem::attribute_value(const std::string&
	attribname) const
{
	if (attribname == "title") {
		return utils::utf8_to_locale(title());
	} else if (attribname == "link") {
		return link();
	} else if (attribname == "author") {
		return utils::utf8_to_locale(author());
	} else if (attribname == "content") {
		ScopeMeasure sm("RssItem::attribute_value(\"content\")");
		std::lock_guard<std::mutex> guard(description_mutex);
		if (description_.has_value()) {
			const std::string content = description_.value().text;
			return utils::utf8_to_locale(content);
		} else if (ch) {
			std::string description = ch->fetch_description(*this);
			return utils::utf8_to_locale(description);
		}
		return "";
	} else if (attribname == "date") {
		return pubDate();
	} else if (attribname == "guid") {
		return guid();
	} else if (attribname == "unread") {
		return unread_ ? "yes" : "no";
	} else if (attribname == "enclosure_url") {
		return enclosure_url();
	} else if (attribname == "enclosure_type") {
		return enclosure_type();
	} else if (attribname == "flags") {
		return flags();
	} else if (attribname == "age")
		return std::to_string(
				(time(nullptr) - pubDate_timestamp()) / 86400);
	else if (attribname == "articleindex") {
		return std::to_string(idx);
	}

	// if we have a feed, then forward the request
	std::shared_ptr<RssFeed> feedptr = feedptr_.lock();
	if (feedptr) {
		return feedptr->RssFeed::attribute_value(attribname);
	}

	return std::nullopt;
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
	[](const char c) {
		return !isalpha(c);
	}),
	flags_.end());

	// Erase doubled characters
	flags_.erase(std::unique(flags_.begin(), flags_.end()), flags_.end());
}

void RssItem::set_feedptr(std::shared_ptr<RssFeed> ptr)
{
	feedptr_ = std::weak_ptr<RssFeed>(ptr);
}

void RssItem::set_feedptr(const std::weak_ptr<RssFeed>& ptr)
{
	feedptr_ = ptr;
}

} // namespace newsboat

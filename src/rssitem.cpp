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
{
}

// RssItem setters

void RssItem::set_guid(const std::string& g)
{
	guid_ = g;
}

void RssItem::set_unread_nowrite_notify(bool u, bool notify)
{
	std::shared_ptr<RssFeed> feedptr = feedptr_.lock();
	if (feedptr && notify) {
		feedptr->get_item_by_guid(guid_)->set_unread_nowrite(u); // notify parent feed
	}
}

void RssItem::set_unread(bool u)
{
		std::shared_ptr<RssFeed> feedptr = feedptr_.lock();
		if (feedptr)
			feedptr->get_item_by_guid(guid_)->set_unread_nowrite(u); // notify parent feed
		try {
			if (ch) {
				ch->update_rssitem_unread_and_enqueued(
					*this, feedurl_);
			}
		} catch (const DbException& e) {
			// if the update failed, restore the old unread flag and
			// rethrow the exception
			throw;
		}
}

const std::string& RssItem::feedurl() const
{
	return feedurl_;
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
		return "";
	} else if (attribname == "date") {
		return pubDate();
	} else if (attribname == "guid") {
		return guid();
	} else if (attribname == "unread") {
		return "yes";
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

void RssItem::set_feedptr(std::shared_ptr<RssFeed> ptr)
{
	feedptr_ = std::weak_ptr<RssFeed>(ptr);
}

void RssItem::set_feedptr(const std::weak_ptr<RssFeed>& ptr)
{
	feedptr_ = ptr;
}

} // namespace newsboat

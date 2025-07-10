#include "rssparser.h"

#include <algorithm>
#include <cinttypes>
#include <curl/curl.h>

#include "cache.h"
#include "configcontainer.h"
#include "curlhandle.h"
#include "htmlrenderer.h"
#include "logger.h"
#include "rss/parser.h"
#include "rss/rssparser.h"
#include "rssfeed.h"
#include "rssignores.h"
#include "utils.h"

namespace Newsboat {

RssParser::RssParser(const std::string& uri,
	Cache& c,
	ConfigContainer& cfg,
	RssIgnores* ii)
	: my_uri(uri)
	, ch(c)
	, cfgcont(cfg)
	, ign(ii)
{
}

RssParser::~RssParser() {}

std::shared_ptr<RssFeed> RssParser::parse(const rsspp::Feed& upstream_feed)
{
	if (upstream_feed.rss_version == rsspp::Feed::Version::UNKNOWN) {
		return nullptr;
	}

	std::shared_ptr<RssFeed> feed(new RssFeed(&ch, my_uri));

	/*
	 * After parsing is done, we fill our feed object with title,
	 * description, etc.  It's important to note that all data that
	 * comes from rsspp must be converted to UTF-8 before, because
	 * all data is internally stored as UTF-8, and converted
	 * on-the-fly in case some other encoding is required. This is
	 * because UTF-8 can hold all available Unicode characters,
	 * unlike other non-Unicode encodings.
	 */

	fill_feed_fields(feed, upstream_feed);
	fill_feed_items(feed, upstream_feed);

	ch.remove_old_deleted_items(feed.get());

	return feed;
}

time_t RssParser::parse_date(const std::string& datestr)
{
	time_t t = curl_getdate(datestr.c_str(), nullptr);
	if (t == -1) {
		LOG(Level::INFO,
			"RssParser::parse_date: encountered t == -1, trying "
			"out "
			"W3CDTF parser...");
		t = curl_getdate(
				rsspp::RssParser::w3cdtf_to_rfc822(datestr).c_str(),
				nullptr);
	}
	if (t == -1) {
		LOG(Level::INFO,
			"RssParser::parse_date: still t == -1, setting to "
			"current "
			"time");
		t = ::time(nullptr);
	}
	return t;
}

void RssParser::replace_newline_characters(std::string& str)
{
	str = utils::replace_all(str, "\r", " ");
	str = utils::replace_all(str, "\n", " ");
	utils::trim(str);
}

std::string RssParser::render_xhtml_title(const std::string& title,
	const std::string& link)
{
	HtmlRenderer rnd(true);
	std::vector<std::pair<LineType, std::string>> lines;
	Links links; // not needed
	rnd.render(title, lines, links, link);
	if (!lines.empty()) {
		return lines[0].second;
	}
	return "";
}

void RssParser::set_rtl(std::shared_ptr<RssFeed> feed,
	const std::string& lang)
{
	// we implement right-to-left support for the languages listed in
	// http://blogs.msdn.com/rssteam/archive/2007/05/17/reading-feeds-in-right-to-left-order.aspx
	static const std::unordered_set<std::string> rtl_langprefix{
		"ar",  // Arabic
		"fa",  // Farsi
		"ur",  // Urdu
		"ps",  // Pashtu
		"syr", // Syriac
		"dv",  // Divehi
		"he",  // Hebrew
		"yi"   // Yiddish
	};
	auto it = rtl_langprefix.find(lang);
	if (it != rtl_langprefix.end()) {
		LOG(Level::DEBUG,
			"RssParser::parse: detected right-to-left order, "
			"language "
			"code = %s",
			*it);
		feed->set_rtl(true);
	}
}

void RssParser::fill_feed_fields(std::shared_ptr<RssFeed> feed,
	const rsspp::Feed& upstream_feed)
{
	/*
	 * we fill all the feed members with the appropriate values from the
	 * rsspp data structure
	 */
	if (is_html_type(upstream_feed.title_type)) {
		feed->set_title(render_xhtml_title(upstream_feed.title, feed->link()));
	} else {
		feed->set_title(upstream_feed.title);
	}

	feed->set_description(upstream_feed.description);

	feed->set_link(utils::absolute_url(my_uri, upstream_feed.link));

	if (!upstream_feed.pubDate.empty()) {
		feed->set_pubDate(parse_date(upstream_feed.pubDate));
	} else {
		feed->set_pubDate(::time(nullptr));
	}

	set_rtl(feed, upstream_feed.language);

	LOG(Level::DEBUG,
		"RssParser::parse: feed title = `%s' link = `%s'",
		feed->title(),
		feed->link());
}

void RssParser::fill_feed_items(std::shared_ptr<RssFeed> feed,
	const rsspp::Feed& upstream_feed)
{
	/*
	 * we iterate over all items of a feed, create an RssItem object for
	 * each item, and fill it with the appropriate values from the data
	 * structure.
	 */
	for (const auto& item : upstream_feed.items) {
		std::shared_ptr<RssItem> x(new RssItem(&ch));

		set_item_title(feed, x, item);

		if (!item.link.empty()) {
			x->set_link(
				utils::absolute_url(feed->link(), item.link));
		}

		if (x->link().empty() && item.guid_isPermaLink) {
			x->set_link(item.guid);
		}

		set_item_author(x, item, upstream_feed);

		x->set_feedurl(feed->rssurl());
		x->set_feedptr(feed);

		// TODO: replace this with a switch to get compiler errors when new
		// entry is added to the enum.
		if ((upstream_feed.rss_version == rsspp::Feed::ATOM_1_0 ||
				upstream_feed.rss_version == rsspp::Feed::TTRSS_JSON ||
				upstream_feed.rss_version == rsspp::Feed::NEWSBLUR_JSON ||
				upstream_feed.rss_version == rsspp::Feed::OCNEWS_JSON ||
				upstream_feed.rss_version == rsspp::Feed::MINIFLUX_JSON ||
				upstream_feed.rss_version == rsspp::Feed::FEEDBIN_JSON ||
				upstream_feed.rss_version == rsspp::Feed::FRESHRSS_JSON) &&
			item.labels.size() > 0) {
			auto start = item.labels.begin();
			auto finish = item.labels.end();

			if (std::find(start, finish, "fresh") != finish) {
				x->set_unread_nowrite(true);
				x->set_override_unread(true);
			}
			if (std::find(start, finish, "kept-unread") != finish) {
				x->set_unread_nowrite(true);
				x->set_override_unread(true);
			}
			if (std::find(start, finish, "read") != finish) {
				x->set_unread_nowrite(false);
				x->set_override_unread(true);
			}
			if (std::find(start, finish, "ttrss:unread") !=
				finish) {
				x->set_unread_nowrite(true);
				x->set_override_unread(true);
			}
			if (std::find(start, finish, "ttrss:read") != finish) {
				x->set_unread_nowrite(false);
				x->set_override_unread(true);
			}
			if (std::find(start, finish, "newsblur:unread") !=
				finish) {
				x->set_unread_nowrite(true);
				x->set_override_unread(true);
			}
			if (std::find(start, finish, "newsblur:read") !=
				finish) {
				x->set_unread_nowrite(false);
				x->set_override_unread(true);
			}
			if (std::find(start, finish, "ocnews:unread") !=
				finish) {
				x->set_unread_nowrite(true);
				x->set_override_unread(true);
			}
			if (std::find(start, finish, "ocnews:read") != finish) {
				x->set_unread_nowrite(false);
				x->set_override_unread(true);
			}
			if (std::find(start, finish, "miniflux:unread") !=
				finish) {
				x->set_unread_nowrite(true);
				x->set_override_unread(true);
			}
			if (std::find(start, finish, "miniflux:read") != finish) {
				x->set_unread_nowrite(false);
				x->set_override_unread(true);
			}
			if (std::find(start, finish, "feedbin:unread") !=
				finish) {
				x->set_unread_nowrite(true);
				x->set_override_unread(true);
			}
			if (std::find(start, finish, "feedbin:read") != finish) {
				x->set_unread_nowrite(false);
				x->set_override_unread(true);
			}
		}

		set_item_content(x, item);

		if (!item.pubDate.empty()) {
			x->set_pubDate(parse_date(item.pubDate));
		} else {
			x->set_pubDate(::time(nullptr));
		}

		x->set_guid(get_guid(item));

		x->set_base(item.base);

		set_item_enclosure(x, item);

		LOG(Level::DEBUG,
			"RssParser::parse: item title = `%s' link = `%s' "
			"pubDate "
			"= `%s' (%" PRId64 ") description = `%s'",
			x->title(),
			x->link(),
			x->pubDate(),
			// On GCC, `time_t` is `long int`, which is at least 32 bits long
			// according to the spec. On x86_64, it's actually 64 bits. Thus,
			// casting to int64_t is either a no-op, or an up-cast which are
			// always safe.
			static_cast<int64_t>(x->pubDate_timestamp()),
			x->description().text);

		add_item_to_feed(feed, x);
	}
}

void RssParser::set_item_title(std::shared_ptr<RssFeed> feed,
	std::shared_ptr<RssItem> x,
	const rsspp::Item& item)
{
	std::string title = item.title;

	if (title.empty()) {
		title = utils::make_title(item.link);
		if (title.empty()) {
			if (!item.description.empty()) {
				x->set_title(render_xhtml_title(item.description, feed->link()));
				return;
			}
			if (!item.content_encoded.empty()) {
				x->set_title(render_xhtml_title(item.content_encoded, feed->link()));
				return;
			}
		}
	}

	if (is_html_type(item.title_type)) {
		x->set_title(render_xhtml_title(title, feed->link()));
	} else {
		replace_newline_characters(title);
		x->set_title(title);
	}
}

void RssParser::set_item_author(std::shared_ptr<RssItem> x,
	const rsspp::Item& item, const rsspp::Feed& upstream_feed)
{
	/*
	 * some feeds only have a feed-wide managingEditor, which we use as an
	 * item's author if there is no item-specific one available.
	 */
	std::string author = item.author;
	if (author.empty()) {
		author = upstream_feed.managingeditor;
	}
	if (author.empty()) {
		author = upstream_feed.dc_creator;
	}
	replace_newline_characters(author);
	x->set_author(author);
}

void RssParser::set_item_content(std::shared_ptr<RssItem> x,
	const rsspp::Item& item)
{
	handle_content_encoded(x, item);

	handle_itunes_summary(x, item);

	if (x->description().text.empty()) {
		x->set_description(item.description, item.description_mime_type);
	} else {
		if (cfgcont.get_configvalue_as_bool(
				"always-display-description") &&
			!item.description.empty())
			x->set_description(
				x->description().text + "<hr>" + item.description, "text/html");
	}

	/* if it's still empty and we shall download the full page, then we do
	 * so. */
	if (x->description().text.empty() &&
		cfgcont.get_configvalue_as_bool("download-full-page") &&
		!x->link().empty()) {

		CurlHandle handle;
		const std::string content = utils::retrieve_url(x->link(), handle, cfgcont, "", nullptr,
				utils::HTTPMethod::GET);
		std::string content_mime_type;

		// Determine mime-type based on Content-type header:
		// Content-type: https://tools.ietf.org/html/rfc7231#section-3.1.1.5
		// Format: https://tools.ietf.org/html/rfc7231#section-3.1.1.1
		char* value = nullptr;
		curl_easy_getinfo(handle.ptr(), CURLINFO_CONTENT_TYPE, &value);
		if (value != nullptr) {
			std::string content_type(value);
			content_mime_type = content_type.substr(0, content_type.find_first_of(";"));
		} else {
			content_mime_type = "application/octet-stream";
		}

		x->set_description(content, content_mime_type);
	}

	LOG(Level::DEBUG,
		"RssParser::set_item_content: content = %s",
		x->description().text);
}

std::string RssParser::get_guid(const rsspp::Item& item) const
{
	/*
	 * We try to find a GUID (some unique identifier) for an item. If the
	 * regular GUID is not available (oh, well, there are a few broken feeds
	 * around, after all), we try out the link and the title, instead. This
	 * is suboptimal, of course, because it makes it impossible to recognize
	 * duplicates when the title or the link changes.
	 */
	if (!item.guid.empty()) {
		return item.guid;
	} else if (!item.link.empty() && !item.pubDate.empty()) {
		return item.link + item.pubDate;
	} else if (!item.link.empty()) {
		return item.link;
	} else if (!item.title.empty()) {
		return item.title;
	} else {
		return "";        // too bad.
	}
}

void RssParser::set_item_enclosure(std::shared_ptr<RssItem> x,
	const rsspp::Item& item)
{
	std::string enclosure_url;
	std::string enclosure_type;
	std::string enclosure_description;
	std::string enclosure_description_mime_type;
	bool found_valid_enclosure = false;

	for (const auto& enclosure : item.enclosures) {
		if (utils::is_valid_podcast_type(enclosure.type)) {
			found_valid_enclosure = true;
			enclosure_url = enclosure.url;
			enclosure_type = enclosure.type;
			enclosure_description = enclosure.description;
			enclosure_description_mime_type = enclosure.description_mime_type;
		} else if (!found_valid_enclosure) {
			enclosure_url = enclosure.url;
			enclosure_type = enclosure.type;
			enclosure_description = enclosure.description;
			enclosure_description_mime_type = enclosure.description_mime_type;
		}
	}

	x->set_enclosure_url(enclosure_url);
	x->set_enclosure_type(enclosure_type);
	x->set_enclosure_description(enclosure_description);
	x->set_enclosure_description_mime_type(enclosure_description_mime_type);
	LOG(Level::DEBUG,
		"RssParser::parse: found enclosure_url: %s",
		enclosure_url);
	LOG(Level::DEBUG,
		"RssParser::parse: found enclosure_type: %s",
		enclosure_type);
}

void RssParser::add_item_to_feed(std::shared_ptr<RssFeed> feed,
	std::shared_ptr<RssItem> item)
{
	// only add item to feed if it isn't on the ignore list or if there is
	// no ignore list
	if (!ign || !ign->matches(item.get())) {
		feed->add_item(item);
		LOG(Level::INFO,
			"RssParser::parse: added article title = `%s' link = "
			"`%s' "
			"ign = %p",
			item->title(),
			item->link(),
			ign);
	} else {
		LOG(Level::INFO,
			"RssParser::parse: ignored article title = `%s' link "
			"= "
			"`%s'",
			item->title(),
			item->link());
	}
}

void RssParser::handle_content_encoded(std::shared_ptr<RssItem> x,
	const rsspp::Item& item) const
{
	if (!x->description().text.empty()) {
		return;
	}

	/* here we handle content:encoded tags that are an extension but very
	 * widespread */
	if (!item.content_encoded.empty()) {
		x->set_description(item.content_encoded, "text/html");
	} else {
		LOG(Level::DEBUG,
			"RssParser::parse: found no content:encoded");
	}
}

void RssParser::handle_itunes_summary(std::shared_ptr<RssItem> x,
	const rsspp::Item& item)
{
	if (!x->description().text.empty()) {
		return;
	}

	std::string summary = item.itunes_summary;
	if (!summary.empty()) {
		std::string desc = "<ituneshack>";
		desc.append(summary);
		desc.append("</ituneshack>");
		x->set_description(desc, "text/html");
	}
}

bool RssParser::is_html_type(const std::string& type)
{
	return (type == "html" || type == "xhtml" ||
			type == "application/xhtml+xml");
}

} // namespace Newsboat

#include "rssparser.h"

#include <algorithm>
#include <cerrno>
#include <cinttypes>
#include <cstring>
#include <curl/curl.h>
#include <sstream>

#include "cache.h"
#include "config.h"
#include "configcontainer.h"
#include "curlhandle.h"
#include "htmlrenderer.h"
#include "logger.h"
#include "minifluxapi.h"
#include "newsblurapi.h"
#include "freshrssapi.h"
#include "ocnewsapi.h"
#include "rss/exception.h"
#include "rss/parser.h"
#include "rss/rssparser.h"
#include "rssfeed.h"
#include "rssignores.h"
#include "strprintf.h"
#include "ttrssapi.h"
#include "utils.h"

namespace newsboat {

RssParser::RssParser(const std::string& uri,
	Cache* c,
	ConfigContainer* cfg,
	RssIgnores* ii,
	RemoteApi* a)
	: my_uri(uri)
	, ch(c)
	, cfgcont(cfg)
	, ign(ii)
	, api(a)
	, easyhandle(0)
{
	is_ttrss = cfgcont->get_configvalue("urls-source") == "ttrss";
	is_newsblur = cfgcont->get_configvalue("urls-source") == "newsblur";
	is_ocnews = cfgcont->get_configvalue("urls-source") == "ocnews";
	is_miniflux = cfgcont->get_configvalue("urls-source") == "miniflux";
    is_freshrss = cfgcont->get_configvalue("urls-source") == "freshrss";
}

RssParser::~RssParser() {}

std::shared_ptr<RssFeed> RssParser::parse()
{
	retrieve_uri(my_uri);

	if (f.rss_version == rsspp::Feed::Version::UNKNOWN) {
		return nullptr;
	}

	std::shared_ptr<RssFeed> feed(new RssFeed(ch, my_uri));

	/*
	 * After parsing is done, we fill our feed object with title,
	 * description, etc.  It's important to note that all data that
	 * comes from rsspp must be converted to UTF-8 before, because
	 * all data is internally stored as UTF-8, and converted
	 * on-the-fly in case some other encoding is required. This is
	 * because UTF-8 can hold all available Unicode characters,
	 * unlike other non-Unicode encodings.
	 */

	fill_feed_fields(feed);
	fill_feed_items(feed);

	ch->remove_old_deleted_items(feed.get());

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
	std::vector<LinkPair> links; // not needed
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

void RssParser::retrieve_uri(const std::string& uri)
{
	/*
	 *	- http:// and https:// URLs are downloaded and parsed regularly
	 *	- exec: URLs are executed and their output is parsed
	 *	- filter: URLs are downloaded, executed, and their output is
	 *parsed
	 *	- query: URLs are ignored
	 */
	if (is_ttrss) {
		std::string::size_type pound = uri.find_first_of('#');
		if (pound != std::string::npos) {
			fetch_ttrss(my_uri.substr(pound + 1));
		}
	} else if (is_newsblur) {
		fetch_newsblur(uri);
	} else if (is_ocnews) {
		fetch_ocnews(uri);
	} else if (is_miniflux) {
		fetch_miniflux(uri);
    } else if (is_freshrss) {
        fetch_freshrss(uri);
	} else if (utils::is_http_url(uri)) {
		download_http(uri);
	} else if (utils::is_exec_url(uri)) {
		get_execplugin(uri.substr(5, uri.length() - 5));
	} else if (utils::is_filter_url(uri)) {
		const auto parts = utils::extract_filter(uri);
		download_filterplugin(std::string(parts.script_name), std::string(parts.url));
	} else if (utils::is_query_url(my_uri)) {
		f.rss_version = rsspp::Feed::Version::UNKNOWN;
	} else if (my_uri.substr(0, 7) == "file://") {
		parse_file(my_uri.substr(7, my_uri.length() - 7));
	} else {
		throw strprintf::fmt(_("Error: unsupported URL: %s"), my_uri);
	}
}

void RssParser::download_http(const std::string& uri)
{
	unsigned int retrycount =
		cfgcont->get_configvalue_as_int("download-retries");
	std::string proxy;
	std::string proxy_auth;
	std::string proxy_type;

	if (cfgcont->get_configvalue_as_bool("use-proxy") == true) {
		proxy = cfgcont->get_configvalue("proxy");
		proxy_auth = cfgcont->get_configvalue("proxy-auth");
		proxy_type = cfgcont->get_configvalue("proxy-type");
	}

	for (unsigned int i = 0; i < retrycount
		&& f.rss_version == rsspp::Feed::Version::UNKNOWN; i++) {
		std::string useragent = utils::get_useragent(cfgcont);
		LOG(Level::DEBUG,
			"RssParser::download_http: user-agent = %s",
			useragent);
		rsspp::Parser p(cfgcont->get_configvalue_as_int(
				"download-timeout"),
			useragent,
			proxy,
			proxy_auth,
			utils::get_proxy_type(proxy_type),
			cfgcont->get_configvalue_as_bool(
				"ssl-verifypeer"));
		time_t lm = 0;
		std::string etag;
		if (!ign || !ign->matches_lastmodified(uri)) {
			ch->fetch_lastmodified(uri, lm, etag);
		}
		f = p.parse_url(uri,
				lm,
				etag,
				api,
				cfgcont->get_configvalue("cookie-cache"),
				easyhandle ? easyhandle->ptr() : 0);
		LOG(Level::DEBUG,
			"RssParser::download_http: lm = %" PRId64 " etag = %s",
			// On GCC, `time_t` is `long int`, which is at least 32 bits
			// long according to the spec. On x86_64, it's actually 64
			// bits. Thus, casting to int64_t is either a no-op, or an
			// up-cast which are always safe.
			static_cast<int64_t>(p.get_last_modified()),
			p.get_etag());
		if (p.get_last_modified() != 0 ||
			p.get_etag().length() > 0) {
			LOG(Level::DEBUG,
				"RssParser::download_http: "
				"lastmodified "
				"old: %" PRId64 " new: %" PRId64,
				// On GCC, `time_t` is `long int`, which is at least 32
				// bits long according to the spec. On x86_64, it's
				// actually 64 bits. Thus, casting to int64_t is either
				// a no-op, or an up-cast which are always safe.
				static_cast<int64_t>(lm),
				static_cast<int64_t>(p.get_last_modified()));
			LOG(Level::DEBUG,
				"RssParser::download_http: etag old: "
				"%s "
				"new %s",
				etag,
				p.get_etag());
			ch->update_lastmodified(uri,
				(p.get_last_modified() != lm)
				? p.get_last_modified()
				: 0,
				(etag != p.get_etag()) ? p.get_etag()
				: "");
		}
	}
	LOG(Level::DEBUG,
		"RssParser::parse: http URL %s, valid: %s",
		uri,
		(f.rss_version != rsspp::Feed::Version::UNKNOWN) ? "true" : "false");
}

void RssParser::get_execplugin(const std::string& plugin)
{
	std::string buf = utils::get_command_output(plugin);
	rsspp::Parser p;
	f = p.parse_buffer(buf);
	LOG(Level::DEBUG,
		"RssParser::parse: execplugin %s, valid = %s",
		plugin,
		(f.rss_version != rsspp::Feed::Version::UNKNOWN) ? "true" : "false");
}

void RssParser::parse_file(const std::string& file)
{
	rsspp::Parser p;
	f = p.parse_file(file);
	LOG(Level::DEBUG,
		"RssParser::parse: parsed file %s, valid = %s",
		file,
		(f.rss_version != rsspp::Feed::Version::UNKNOWN) ? "true" : "false");
}

void RssParser::download_filterplugin(const std::string& filter,
	const std::string& uri)
{
	std::string buf = utils::retrieve_url(uri, cfgcont);

	const char* argv[4] = {"/bin/sh",
			"-c",
			filter.c_str(),
			nullptr
		};
	std::string result = utils::run_program(argv, buf);
	LOG(Level::DEBUG,
		"RssParser::parse: output of `%s' is: %s",
		filter,
		result);
	rsspp::Parser p;
	f = p.parse_buffer(result);
	LOG(Level::DEBUG,
		"RssParser::parse: filterplugin %s, valid = %s",
		filter,
		(f.rss_version != rsspp::Feed::Version::UNKNOWN) ? "true" : "false");
}

void RssParser::fill_feed_fields(std::shared_ptr<RssFeed> feed)
{
	/*
	 * we fill all the feed members with the appropriate values from the
	 * rsspp data structure
	 */
	if (is_html_type(f.title_type)) {
		feed->set_title(render_xhtml_title(f.title, feed->link()));
	} else {
		feed->set_title(f.title);
	}

	feed->set_description(f.description);

	feed->set_link(utils::absolute_url(my_uri, f.link));

	if (!f.pubDate.empty()) {
		feed->set_pubDate(parse_date(f.pubDate));
	} else {
		feed->set_pubDate(::time(nullptr));
	}

	set_rtl(feed, f.language);

	LOG(Level::DEBUG,
		"RssParser::parse: feed title = `%s' link = `%s'",
		feed->title(),
		feed->link());
}

void RssParser::fill_feed_items(std::shared_ptr<RssFeed> feed)
{
	/*
	 * we iterate over all items of a feed, create an RssItem object for
	 * each item, and fill it with the appropriate values from the data
	 * structure.
	 */
	for (const auto& item : f.items) {
		std::shared_ptr<RssItem> x(new RssItem(ch));

		set_item_title(feed, x, item);

		if (!item.link.empty()) {
			x->set_link(
				utils::absolute_url(feed->link(), item.link));
		}

		if (x->link().empty() && item.guid_isPermaLink) {
			x->set_link(item.guid);
		}

		set_item_author(x, item);

		x->set_feedurl(feed->rssurl());
		x->set_feedptr(feed);

		// TODO: replace this with a switch to get compiler errors when new
		// entry is added to the enum.
		if ((f.rss_version == rsspp::Feed::ATOM_1_0 ||
				f.rss_version == rsspp::Feed::TTRSS_JSON ||
				f.rss_version == rsspp::Feed::NEWSBLUR_JSON ||
				f.rss_version == rsspp::Feed::OCNEWS_JSON ||
                f.rss_version == rsspp::Feed::MINIFLUX_JSON ||
				f.rss_version == rsspp::Feed::FRESHRSS_JSON) &&
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

	if (item.title.empty()) {
		title = utils::make_title(item.link);
	}

	if (is_html_type(item.title_type)) {
		x->set_title(render_xhtml_title(title, feed->link()));
	} else {
		replace_newline_characters(title);
		x->set_title(title);
	}
}

void RssParser::set_item_author(std::shared_ptr<RssItem> x,
	const rsspp::Item& item)
{
	/*
	 * some feeds only have a feed-wide managingEditor, which we use as an
	 * item's author if there is no item-specific one available.
	 */
	if (item.author.empty()) {
		if (!f.managingeditor.empty()) {
			x->set_author(f.managingeditor);
		} else {
			x->set_author(f.dc_creator);
		}
	} else {
		x->set_author(item.author);
	}
}

void RssParser::set_item_content(std::shared_ptr<RssItem> x,
	const rsspp::Item& item)
{
	handle_content_encoded(x, item);

	handle_itunes_summary(x, item);

	if (x->description().text.empty()) {
		x->set_description(item.description, item.description_mime_type);
	} else {
		if (cfgcont->get_configvalue_as_bool(
				"always-display-description") &&
			!item.description.empty())
			x->set_description(
				x->description().text + "<hr>" + item.description, "text/html");
	}

	/* if it's still empty and we shall download the full page, then we do
	 * so. */
	if (x->description().text.empty() &&
		cfgcont->get_configvalue_as_bool("download-full-page") &&
		!x->link().empty()) {

		CURL* easyhandle = curl_easy_init();
		const std::string content = utils::retrieve_url(x->link(), cfgcont, "", nullptr,
				HTTPMethod::GET, easyhandle);
		std::string content_mime_type;

		// Determine mime-type based on Content-type header:
		// Content-type: https://tools.ietf.org/html/rfc7231#section-3.1.1.5
		// Format: https://tools.ietf.org/html/rfc7231#section-3.1.1.1
		char* value = nullptr;
		curl_easy_getinfo(easyhandle, CURLINFO_CONTENT_TYPE, &value);
		if (value != nullptr) {
			std::string content_type(value);
			content_mime_type = content_type.substr(0, content_type.find_first_of(";"));
		} else {
			content_mime_type = "application/octet-stream";
		}

		x->set_description(content, content_mime_type);
		curl_easy_cleanup(easyhandle);
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
	x->set_enclosure_url(item.enclosure_url);
	x->set_enclosure_type(item.enclosure_type);
	LOG(Level::DEBUG,
		"RssParser::parse: found enclosure_url: %s",
		item.enclosure_url);
	LOG(Level::DEBUG,
		"RssParser::parse: found enclosure_type: %s",
		item.enclosure_type);
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

void RssParser::fetch_ttrss(const std::string& feed_id)
{
	TtRssApi* tapi = dynamic_cast<TtRssApi*>(api);
	if (tapi) {
		f = tapi->fetch_feed(
				feed_id, easyhandle ? easyhandle->ptr() : nullptr);
	}
	LOG(Level::DEBUG,
		"RssParser::fetch_ttrss: f.items.size = %" PRIu64,
		static_cast<uint64_t>(f.items.size()));
}

void RssParser::fetch_newsblur(const std::string& feed_id)
{
	NewsBlurApi* napi = dynamic_cast<NewsBlurApi*>(api);
	if (napi) {
		f = napi->fetch_feed(feed_id);
	}
	LOG(Level::INFO,
		"RssParser::fetch_newsblur: f.items.size = %" PRIu64,
		static_cast<uint64_t>(f.items.size()));
}

void RssParser::fetch_ocnews(const std::string& feed_id)
{
	OcNewsApi* napi = dynamic_cast<OcNewsApi*>(api);
	if (napi) {
		f = napi->fetch_feed(feed_id);
	}
	LOG(Level::INFO,
		"RssParser::fetch_ocnews: f.items.size = %" PRIu64,
		static_cast<uint64_t>(f.items.size()));
}

void RssParser::fetch_miniflux(const std::string& feed_id)
{
	MinifluxApi* mapi = dynamic_cast<MinifluxApi*>(api);
	if (mapi) {
		f = mapi->fetch_feed(feed_id, easyhandle ? easyhandle->ptr() : nullptr);
	}
	LOG(Level::INFO,
		"RssParser::fetch_miniflux: f.items.size = %" PRIu64,
		static_cast<uint64_t>(f.items.size()));
}

void RssParser::fetch_freshrss(const std::string& feed_id)
{
    FreshRssApi* fapi = dynamic_cast<FreshRssApi*>(api);
    if (fapi) {
        f = fapi->fetch_feed(feed_id, easyhandle ? easyhandle->ptr() : nullptr);
    }
    LOG(Level::INFO,
        "RssParser::fetch_freshrss: f.items.size = %" PRIu64,
        static_cast<uint64_t>(f.items.size()));
}

} // namespace newsboat

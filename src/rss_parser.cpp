#include <rsspp.h>
#include <rsspp_internal.h>

#include <rss_parser.h>
#include <configcontainer.h>
#include <cache.h>
#include <rss.h>
#include <logger.h>
#include <utils.h>
#include <config.h>
#include <htmlrenderer.h>
#include <ttrss_api.h>
#include <newsblur_api.h>
#include <curl/curl.h>

#include <cerrno>
#include <cstring>
#include <sstream>
#include <algorithm>

namespace newsbeuter {

rss_parser::rss_parser(const std::string& uri, cache * c, configcontainer * cfg, rss_ignores * ii, remote_api * a) 
	: my_uri(uri), ch(c), cfgcont(cfg), skip_parsing(false), is_valid(false), ign(ii), api(a), easyhandle(0) { 
	is_ttrss = cfgcont->get_configvalue("urls-source") == "ttrss";
	is_newsblur = cfgcont->get_configvalue("urls-source") == "newsblur";
}

rss_parser::~rss_parser() { }

std::shared_ptr<rss_feed> rss_parser::parse() {
	std::shared_ptr<rss_feed> feed(new rss_feed(ch));

	feed->set_rssurl(my_uri);

	retrieve_uri(my_uri);

	if (!skip_parsing && is_valid) {

		/*
		 * After parsing is done, we fill our feed object with title,
		 * description, etc.  It's important to note that all data that comes
		 * from rsspp must be converted to UTF-8 before, because all data is
		 * internally stored as UTF-8, and converted on-the-fly in case some
		 * other encoding is required. This is because UTF-8 can hold all
		 * available Unicode characters, unlike other non-Unicode encodings.
		 */

		fill_feed_fields(feed);
		fill_feed_items(feed);

		feed->remove_old_deleted_items();
	}

	feed->set_empty(false);

	return feed;
}

time_t rss_parser::parse_date(const std::string& datestr) {
	time_t t = curl_getdate(datestr.c_str(), NULL);
	if (t == -1) {
		LOG(LOG_INFO, "rss_parser::parse_date: encountered t == -1, trying out W3CDTF parser...");
		t = curl_getdate(rsspp::rss_parser::__w3cdtf_to_rfc822(datestr).c_str(), NULL);
	}
	if (t == -1) {
		LOG(LOG_INFO, "rss_parser::parse_date: still t == -1, setting to current time");
		t = ::time(NULL);
	}
	return t;
}

void rss_parser::replace_newline_characters(std::string& str) {
	str = utils::replace_all(str, "\r", " ");
	str = utils::replace_all(str, "\n", " ");
	utils::trim(str);
}

std::string rss_parser::render_xhtml_title(const std::string& title, const std::string& link) {
	htmlrenderer rnd(1 << 16, true); // a huge number
	std::vector<std::string> lines;
	std::vector<linkpair> links; // not needed
	rnd.render(title, lines, links, link);
	if (!lines.empty())
		return lines[0];
	return "";
}

void rss_parser::set_rtl(std::shared_ptr<rss_feed> feed, const char * lang) {
	// we implement right-to-left support for the languages listed in
	// http://blogs.msdn.com/rssteam/archive/2007/05/17/reading-feeds-in-right-to-left-order.aspx
	static const char * rtl_langprefix[] = { 
		"ar",  // Arabic
		"fa",  // Farsi
		"ur",  // Urdu
		"ps",  // Pashtu
		"syr", // Syriac
		"dv",  // Divehi
		"he",  // Hebrew
		"yi",  // Yiddish
		NULL };
	for (unsigned int i=0;rtl_langprefix[i]!=NULL;++i) {
		if (strncmp(lang,rtl_langprefix[i],strlen(rtl_langprefix[i]))==0) {
			LOG(LOG_DEBUG, "rss_parser::parse: detected right-to-left order, language code = %s", rtl_langprefix[i]);
			feed->set_rtl(true);
			break;
		}
	}
}

void rss_parser::retrieve_uri(const std::string& uri) {
	/*
	 *	- http:// and https:// URLs are downloaded and parsed regularly
	 *	- exec: URLs are executed and their output is parsed
	 *	- filter: URLs are downloaded, executed, and their output is parsed
	 *	- query: URLs are ignored
	 */
	if (is_ttrss) {
		const char * uri = my_uri.c_str();
		const char * pound = strrchr(uri, '#');
		if (pound != NULL) {
			fetch_ttrss(pound+1);
		}
	} else if (is_newsblur) {
		fetch_newsblur(uri);
	} else if (uri.substr(0,5) == "http:" || uri.substr(0,6) == "https:") {
		download_http(uri);
	} else if (uri.substr(0,5) == "exec:") {
		get_execplugin(uri.substr(5,uri.length()-5));
	} else if (uri.substr(0,7) == "filter:") {
		std::string filter, url;
		utils::extract_filter(uri, filter, url);
		download_filterplugin(filter, url);
	} else if (my_uri.substr(0,6) == "query:") {
		skip_parsing = true;
	} else if (my_uri.substr(0,7) == "file://") {
		parse_file(my_uri.substr(7, my_uri.length()-7));
	} else
		throw utils::strprintf(_("Error: unsupported URL: %s"), my_uri.c_str());
}

void rss_parser::download_http(const std::string& uri) {
	unsigned int retrycount = cfgcont->get_configvalue_as_int("download-retries");
	char * proxy = NULL;
	char * proxy_auth = NULL;
	std::string proxy_type;
	is_valid = false;

	if (cfgcont->get_configvalue_as_bool("use-proxy") == true) {
		proxy = const_cast<char *>(cfgcont->get_configvalue("proxy").c_str());
		proxy_auth = const_cast<char *>(cfgcont->get_configvalue("proxy-auth").c_str());
		proxy_type = cfgcont->get_configvalue("proxy-type");
	}

	for (unsigned int i=0;i<retrycount && !is_valid;i++) {
		try {
			std::string useragent = utils::get_useragent(cfgcont);
			LOG(LOG_DEBUG, "rss_parser::download_http: user-agent = %s", useragent.c_str());
			rsspp::parser p(cfgcont->get_configvalue_as_int("download-timeout"), useragent.c_str(), proxy, proxy_auth, utils::get_proxy_type(proxy_type));
			time_t lm = 0;
			std::string etag;
			if (!ign || !ign->matches_lastmodified(uri)) {
				ch->fetch_lastmodified(uri, lm, etag);
			}
			f = p.parse_url(uri, lm, etag, api, cfgcont->get_configvalue("cookie-cache"), easyhandle ? easyhandle->ptr() : 0);
			LOG(LOG_DEBUG, "rss_parser::download_http: lm = %d etag = %s", p.get_last_modified(), p.get_etag().c_str());
			if (p.get_last_modified() != 0 || p.get_etag().length() > 0) {
				LOG(LOG_DEBUG, "rss_parser::download_http: lastmodified old: %d new: %d", lm, p.get_last_modified());
				LOG(LOG_DEBUG, "rss_parser::download_http: etag old: %s new %s", etag.c_str(), p.get_etag().c_str());
				ch->update_lastmodified(uri, (p.get_last_modified() != lm) ? p.get_last_modified() : 0 , (etag != p.get_etag()) ? p.get_etag() : "");
			}
			is_valid = true;
		} catch (rsspp::exception& e) {
			is_valid = false;
			throw e;
		}
	}
	LOG(LOG_DEBUG, "rss_parser::parse: http URL %s, is_valid = %s", uri.c_str(), is_valid ? "true" : "false");
}

void rss_parser::get_execplugin(const std::string& plugin) {
	std::string buf = utils::get_command_output(plugin);
	is_valid = false;
	try {
		rsspp::parser p;
		f = p.parse_buffer(buf.c_str(), buf.length());
		is_valid = true;
	} catch (rsspp::exception& e) {
		is_valid = false;
		throw e;
	}
	LOG(LOG_DEBUG, "rss_parser::parse: execplugin %s, is_valid = %s", plugin.c_str(), is_valid ? "true" : "false");
}

void rss_parser::parse_file(const std::string& file) {
	is_valid = false;
	try {
		rsspp::parser p;
		f = p.parse_file(file);
		is_valid = true;
	} catch (rsspp::exception& e) {
		is_valid = false;
		throw e;
	}
	LOG(LOG_DEBUG, "rss_parser::parse: parsed file %s, is_valid = %s", file.c_str(), is_valid ? "true" : "false");
}

void rss_parser::download_filterplugin(const std::string& filter, const std::string& uri) {
	std::string buf = utils::retrieve_url(uri, cfgcont);

	char * argv[4] = { const_cast<char *>("/bin/sh"), const_cast<char *>("-c"), const_cast<char *>(filter.c_str()), NULL };
	std::string result = utils::run_program(argv, buf);
	LOG(LOG_DEBUG, "rss_parser::parse: output of `%s' is: %s", filter.c_str(), result.c_str());
	is_valid = false;
	try {
		rsspp::parser p;
		f = p.parse_buffer(result.c_str(), result.length());
		is_valid = true;
	} catch (rsspp::exception& e) {
		is_valid = false;
		throw e;
	}
	LOG(LOG_DEBUG, "rss_parser::parse: filterplugin %s, is_valid = %s", filter.c_str(), is_valid ? "true" : "false");
}

void rss_parser::fill_feed_fields(std::shared_ptr<rss_feed> feed) {
	/*
	 * we fill all the feed members with the appropriate values from the rsspp data structure
	 */
	if (is_html_type(f.title_type)) {
		feed->set_title(render_xhtml_title(f.title, feed->link()));
	} else {
		feed->set_title(f.title);
	}

	feed->set_description(f.description);

	feed->set_link(utils::absolute_url(my_uri, f.link));

	if (f.pubDate != "")
		feed->set_pubDate(parse_date(f.pubDate));
	else
		feed->set_pubDate(::time(NULL));

	set_rtl(feed, f.language.c_str());

	LOG(LOG_DEBUG, "rss_parser::parse: feed title = `%s' link = `%s'", feed->title().c_str(), feed->link().c_str());
}

void rss_parser::fill_feed_items(std::shared_ptr<rss_feed> feed) {
	/*
	 * we iterate over all items of a feed, create an rss_item object for
	 * each item, and fill it with the appropriate values from the data structure.
	 */
	for (auto item : f.items) {
		std::shared_ptr<rss_item> x(new rss_item(ch));

		set_item_title(feed, x, item);

		if (item.link != "") {
			x->set_link(utils::absolute_url(feed->link(), item.link));
		}

		if (x->link().empty() && item.guid_isPermaLink) {
			x->set_link(item.guid);
		}

		set_item_author(x, item);

		x->set_feedurl(feed->rssurl());
		x->set_feedptr(feed);

		if ((f.rss_version == rsspp::ATOM_1_0 || f.rss_version == rsspp::TTRSS_JSON || f.rss_version == rsspp::NEWSBLUR_JSON) && item.labels.size() > 0) {
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
			if (std::find(start, finish, "ttrss:unread") != finish) {
				x->set_unread_nowrite(true);
				x->set_override_unread(true);
			}
			if (std::find(start, finish, "ttrss:read") != finish) {
				x->set_unread_nowrite(false);
				x->set_override_unread(true);
			}
			if (std::find(start, finish, "newsblur:unread") != finish) {
				x->set_unread_nowrite(true);
				x->set_override_unread(true);
			}
			if (std::find(start, finish, "newsblur:read") != finish) {
				x->set_unread_nowrite(false);
				x->set_override_unread(true);
			}
		}

		set_item_content(x, item);

		if (item.pubDate != "") 
			x->set_pubDate(parse_date(item.pubDate));
		else
			x->set_pubDate(::time(NULL));
			
		x->set_guid(get_guid(item));

		x->set_base(item.base);

		set_item_enclosure(x, item);

		LOG(LOG_DEBUG, "rss_parser::parse: item title = `%s' link = `%s' pubDate = `%s' (%d) description = `%s'", x->title().c_str(), 
			x->link().c_str(), x->pubDate().c_str(), x->pubDate_timestamp(), x->description().c_str());

		add_item_to_feed(feed, x);
	}
}

void rss_parser::set_item_title(std::shared_ptr<rss_feed> feed, std::shared_ptr<rss_item> x, rsspp::item& item) {
	if (is_html_type(item.title_type)) {
		x->set_title(render_xhtml_title(item.title, feed->link()));
	} else {
		std::string title = item.title;
		replace_newline_characters(title);
		x->set_title(title);
	}
}

void rss_parser::set_item_author(std::shared_ptr<rss_item> x, rsspp::item& item) {
	/* 
	 * some feeds only have a feed-wide managingEditor, which we use as an item's
	 * author if there is no item-specific one available.
	 */
	if (item.author == "") {
		if (f.managingeditor != "")
			x->set_author(f.managingeditor);
		else {
			x->set_author(f.dc_creator);
		}
	} else {
		x->set_author(item.author);
	}
}

void rss_parser::set_item_content(std::shared_ptr<rss_item> x, rsspp::item& item) {

	handle_content_encoded(x, item);

	handle_itunes_summary(x, item);

	if (x->description() == "") {
		x->set_description(item.description);
	} else {
		if (cfgcont->get_configvalue_as_bool("always-display-description") && item.description != "")
			x->set_description(x->description() + "<hr>" + item.description);
	}

	/* if it's still empty and we shall download the full page, then we do so. */
	if (x->description() == "" && cfgcont->get_configvalue_as_bool("download-full-page") && x->link() != "") {
		x->set_description(utils::retrieve_url(x->link(), cfgcont));
	}

	LOG(LOG_DEBUG, "rss_parser::set_item_content: content = %s", x->description().c_str());
}


std::string rss_parser::get_guid(rsspp::item& item) {
	/*
	 * We try to find a GUID (some unique identifier) for an item. If the regular
	 * GUID is not available (oh, well, there are a few broken feeds around, after
	 * all), we try out the link and the title, instead. This is suboptimal, of course,
	 * because it makes it impossible to recognize duplicates when the title or the
	 * link changes.
	 */
	if (item.guid != "")
		return item.guid;
	else if (item.link != "" && item.pubDate != "")
		return item.link + item.pubDate;
	else if (item.link != "")
		return item.link;
	else if (item.title != "")
		return item.title;
	else
		return "";	// too bad.
}

void rss_parser::set_item_enclosure(std::shared_ptr<rss_item> x, rsspp::item& item) {
	x->set_enclosure_url(item.enclosure_url);
	x->set_enclosure_type(item.enclosure_type);
	LOG(LOG_DEBUG, "rss_parser::parse: found enclosure_url: %s", item.enclosure_url.c_str());
	LOG(LOG_DEBUG, "rss_parser::parse: found enclosure_type: %s", item.enclosure_type.c_str());
}

void rss_parser::add_item_to_feed(std::shared_ptr<rss_feed> feed, std::shared_ptr<rss_item> item) {
	// only add item to feed if it isn't on the ignore list or if there is no ignore list
	if (!ign || !ign->matches(item.get())) {
		feed->add_item(item);
		LOG(LOG_INFO, "rss_parser::parse: added article title = `%s' link = `%s' ign = %p", item->title().c_str(), item->link().c_str(), ign);
	} else {
		LOG(LOG_INFO, "rss_parser::parse: ignored article title = `%s' link = `%s'", item->title().c_str(), item->link().c_str());
	}
}

void rss_parser::handle_content_encoded(std::shared_ptr<rss_item> x, rsspp::item& item) {
	if (x->description() != "")
		return;

	/* here we handle content:encoded tags that are an extension but very widespread */
	if (item.content_encoded != "") {
		x->set_description(item.content_encoded);
	} else {
		LOG(LOG_DEBUG, "rss_parser::parse: found no content:encoded");
	}
}

void rss_parser::handle_itunes_summary(std::shared_ptr<rss_item> x, rsspp::item& item) {
	if (x->description() != "")
		return;

	std::string summary = item.itunes_summary;
	if (summary != "") {
		std::string desc = "<ituneshack>";
		desc.append(summary);
		desc.append("</ituneshack>");
		x->set_description(desc);
	}
}

bool rss_parser::is_html_type(const std::string& type) {
	return (type == "html" || type == "xhtml" || type == "application/xhtml+xml");
}

void rss_parser::fetch_ttrss(const std::string& feed_id) {
	ttrss_api * tapi = dynamic_cast<ttrss_api *>(api);
	if (tapi) {
		f = tapi->fetch_feed(feed_id);
		is_valid = true;
	}
	LOG(LOG_DEBUG, "rss_parser::fetch_ttrss: f.items.size = %u", f.items.size());
}

void rss_parser::fetch_newsblur(const std::string& feed_id) {
	newsblur_api * napi = dynamic_cast<newsblur_api *>(api);
	if (napi) {
		f = napi->fetch_feed(feed_id);
		is_valid = true;
	}
	LOG(LOG_INFO, "rss_parser::fetch_newsblur: f.items.size = %u", f.items.size());
}

}

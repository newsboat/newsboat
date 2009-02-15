#include <rsspp.h>

#include <rss_parser.h>
#include <configcontainer.h>
#include <cache.h>
#include <rss.h>
#include <logger.h>
#include <utils.h>
#include <config.h>
#include <htmlrenderer.h>
#include <curl/curl.h>

#include <cerrno>
#include <cstring>
#include <sstream>

namespace newsbeuter {

rss_parser::rss_parser(const char * uri, cache * c, configcontainer * cfg, rss_ignores * ii) 
	: my_uri(uri), ch(c), cfgcont(cfg), skip_parsing(false), is_valid(false), ign(ii) { }

rss_parser::~rss_parser() { }

std::tr1::shared_ptr<rss_feed> rss_parser::parse() {
	std::tr1::shared_ptr<rss_feed> feed(new rss_feed(ch));

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

static size_t get_lastmodified_header(void *ptr, size_t size, size_t nmemb, time_t * timing) {
	char *header = (char *) ptr;

	if (!strncasecmp ("Last-Modified:", header, 14))
		*timing = curl_getdate (header + 14, NULL);

	return size * nmemb;
}


bool rss_parser::check_and_update_lastmodified() {
	if (my_uri.substr(0,5) != "http:" && my_uri.substr(0,6) != "https:")
		return true;

	if (ign && ign->matches_lastmodified(my_uri)) {
		// LOG(LOG_DEBUG, "rss_parser::check_and_update_lastmodified: found %s on list of URLs that are always downloaded", my_uri.c_str());
		return true;
	}

	time_t oldlm = ch->get_lastmodified(my_uri);
	time_t newlm = 0;

	unsigned int retry_count = cfgcont->get_configvalue_as_int("download-retries");

	unsigned int i;
	CURLcode err = CURLE_OK;
	for (i=0;i<retry_count;i++) {
		CURL* curl = curl_easy_init();
		if (!curl) return false;

		std::string proxy = cfgcont->get_configvalue("proxy");
		std::string proxyauth = cfgcont->get_configvalue("proxy-auth");
		std::string useragent = utils::get_useragent(cfgcont);

		LOG(LOG_DEBUG, "rss_parser::check_and_update_lastmodified: useragent = %s", useragent.c_str());

		curl_easy_setopt(curl, CURLOPT_URL, my_uri.c_str());
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
		curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
		curl_easy_setopt(curl, CURLOPT_ENCODING, "gzip, deflate");
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, cfgcont->get_configvalue_as_int("download-timeout"));
		if (proxy != "")
			curl_easy_setopt(curl, CURLOPT_PROXY, proxy.c_str());
		if (proxyauth != "")
			curl_easy_setopt(curl, CURLOPT_PROXYUSERPWD, proxyauth.c_str());
		curl_easy_setopt(curl, CURLOPT_USERAGENT, useragent.c_str());
		curl_easy_setopt(curl, CURLOPT_HEADERDATA, &newlm);
		curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, get_lastmodified_header);
		curl_easy_setopt(curl, CURLOPT_NOBODY, 1);
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
		curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1);

		err = curl_easy_perform(curl);

		long status = 0;
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);

		LOG(LOG_DEBUG, "rss_parser::check_and_update_lastmodified: status = %ld", status);
		if (status >= 400) {
			LOG(LOG_USERERROR, _("Error: trying to download feed `%s' returned HTTP status code %ld."), my_uri.c_str(), status);
		}

		curl_easy_cleanup(curl);

		LOG(LOG_DEBUG, "rss_parser::check_and_update_lastmodified: err = %u oldlm = %d newlm = %d", err, oldlm, newlm);
		if (err == CURLE_OK)
			break;
	}
	if (i==retry_count && err != CURLE_OK) {
		LOG(LOG_DEBUG, "rss_parser::check_and_update_lastmodified: couldn't get a result after %u retries", retry_count);
		throw rsspp::exception(curl_easy_strerror(err));
	}

	if (newlm <= 0) {
		LOG(LOG_DEBUG, "rss_parser::check_and_update_lastmodified: yes, download (no or invalid Last-Modified header: newlm = %d", newlm);
		return true;
	}

	if (newlm > oldlm) {
		ch->set_lastmodified(my_uri, newlm);
		LOG(LOG_DEBUG, "rss_parser::check_and_update_lastmodified: yes, download (newlm > oldlm)");
		return true;
	}

	LOG(LOG_DEBUG, "rss_parser::check_and_update_lastmodified: no, don't download");
	return false;
}

time_t rss_parser::parse_date(const std::string& datestr) {
	return curl_getdate(datestr.c_str(), NULL);
}

void rss_parser::replace_newline_characters(std::string& str) {
	str = utils::replace_all(str, "\r", " ");
	str = utils::replace_all(str, "\n", " ");
}

std::string rss_parser::render_xhtml_title(const std::string& title, const std::string& link) {
	htmlrenderer rnd(1 << 16, true); // a huge number
	std::vector<std::string> lines;
	std::vector<linkpair> links; // not needed
	rnd.render(title, lines, links, link);
	if (lines.size() > 0)
		return lines[0];
	return "";
}

unsigned int rss_parser::monthname_to_number(const std::string& monthstr) {
	static const char * monthtable[] = { "Jan", "Feb", "Mar", "Apr","May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec", NULL };
	for (unsigned int i=0;monthtable[i]!=NULL;i++) {
		if (monthstr == monthtable[i])
			return i;
	}
	return 0;
}

void rss_parser::set_rtl(std::tr1::shared_ptr<rss_feed> feed, const char * lang) {
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

int rss_parser::correct_year(int year) {
	if (year < 100)
		year += 2000;
	return year - 1900;
}

void rss_parser::retrieve_uri(const std::string& uri) {
	/*
	 *	- http:// and https:// URLs are downloaded and parsed regularly
	 *	- exec: URLs are executed and their output is parsed
	 *	- filter: URLs are downloaded, executed, and their output is parsed
	 *	- query: URLs are ignored
	 */
	if (uri.substr(0,5) == "http:" || uri.substr(0,6) == "https:") {
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
	is_valid = false;

	if (cfgcont->get_configvalue_as_bool("use-proxy") == true) {
		proxy = const_cast<char *>(cfgcont->get_configvalue("proxy").c_str());
		proxy_auth = const_cast<char *>(cfgcont->get_configvalue("proxy-auth").c_str());
	}

	for (unsigned int i=0;i<retrycount && !is_valid;i++) {
		try {
			std::string useragent = utils::get_useragent(cfgcont);
			LOG(LOG_DEBUG, "rss_parser::download_http: user-agent = %s", useragent.c_str());
			rsspp::parser p(cfgcont->get_configvalue_as_int("download-timeout"), useragent.c_str(), proxy, proxy_auth);
			f = p.parse_url(uri);
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
	std::string buf = utils::retrieve_url(uri, utils::get_useragent(cfgcont).c_str(), NULL, cfgcont->get_configvalue_as_int("download-timeout"));

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

void rss_parser::fill_feed_fields(std::tr1::shared_ptr<rss_feed> feed) {
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

void rss_parser::fill_feed_items(std::tr1::shared_ptr<rss_feed> feed) {
	/*
	 * we iterate over all items of a feed, create an rss_item object for
	 * each item, and fill it with the appropriate values from the data structure.
	 */
	for (std::vector<rsspp::item>::iterator item=f.items.begin();item!=f.items.end();item++) {
		std::tr1::shared_ptr<rss_item> x(new rss_item(ch));

		set_item_title(feed, x, *item);

		if (item->link != "") {
			x->set_link(utils::absolute_url(feed->link(), item->link));
		}

		set_item_author(x, *item);

		x->set_feedurl(feed->rssurl());

		set_item_content(x, *item);

		if (item->pubDate != "") 
			x->set_pubDate(parse_date(item->pubDate));
		else
			x->set_pubDate(::time(NULL));
			
		x->set_guid(get_guid(*item));

		set_item_enclosure(x, *item);

		LOG(LOG_DEBUG, "rss_parser::parse: item title = `%s' link = `%s' pubDate = `%s' (%d) description = `%s'", x->title().c_str(), 
			x->link().c_str(), x->pubDate().c_str(), x->pubDate_timestamp(), x->description().c_str());

		add_item_to_feed(feed, x);
	}
}

void rss_parser::set_item_title(std::tr1::shared_ptr<rss_feed> feed, std::tr1::shared_ptr<rss_item> x, rsspp::item& item) {
	if (is_html_type(item.title_type)) {
		x->set_title(render_xhtml_title(item.title, feed->link()));
	} else {
		std::string title = item.title;
		replace_newline_characters(title);
		x->set_title(title);
	}
}

void rss_parser::set_item_author(std::tr1::shared_ptr<rss_item> x, rsspp::item& item) {
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

void rss_parser::set_item_content(std::tr1::shared_ptr<rss_item> x, rsspp::item& item) {

	handle_content_encoded(x, item);

	handle_itunes_summary(x, item);

	if (x->description() == "") {
		x->set_description(item.description);
	} else {
		if (cfgcont->get_configvalue_as_bool("always-display-description") && item.description != "")
			x->set_description(x->description() + "<hr>" + item.description);
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
	else if (item.link != "")
		return item.link;
	else if (item.title != "")
		return item.title;
	else
		return "";	// too bad.
}

void rss_parser::set_item_enclosure(std::tr1::shared_ptr<rss_item> x, rsspp::item& item) {
	x->set_enclosure_url(item.enclosure_url);
	x->set_enclosure_type(item.enclosure_type);
	LOG(LOG_DEBUG, "rss_parser::parse: found enclosure_url: %s", item.enclosure_url.c_str());
	LOG(LOG_DEBUG, "rss_parser::parse: found enclosure_type: %s", item.enclosure_type.c_str());
}

void rss_parser::add_item_to_feed(std::tr1::shared_ptr<rss_feed> feed, std::tr1::shared_ptr<rss_item> item) {
	// only add item to feed if it isn't on the ignore list or if there is no ignore list
	if (!ign || !ign->matches(item.get())) {
		feed->items().push_back(item);
		LOG(LOG_INFO, "rss_parser::parse: added article title = `%s' link = `%s' ign = %p", item->title().c_str(), item->link().c_str(), ign);
	} else {
		LOG(LOG_INFO, "rss_parser::parse: ignored article title = `%s' link = `%s'", item->title().c_str(), item->link().c_str());
	}
}

void rss_parser::handle_content_encoded(std::tr1::shared_ptr<rss_item> x, rsspp::item& item) {
	if (x->description() != "")
		return;

	/* here we handle content:encoded tags that are an extension but very widespread */
	if (item.content_encoded != "") {
		x->set_description(item.content_encoded);
	} else {
		LOG(LOG_DEBUG, "rss_parser::parse: found no content:encoded");
	}
}

void rss_parser::handle_itunes_summary(std::tr1::shared_ptr<rss_item> x, rsspp::item& item) {
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

}

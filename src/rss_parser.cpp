#include <rss_parser.h>
#include <configcontainer.h>
#include <cache.h>
#include <rss.h>
#include <logger.h>
#include <utils.h>
#include <config.h>
#include <htmlrenderer.h>

#include <cerrno>
#include <cstring>
#include <sstream>

namespace newsbeuter {

rss_parser::rss_parser(const char * uri, cache * c, configcontainer * cfg, rss_ignores * ii) 
	: my_uri(uri), ch(c), cfgcont(cfg), mrss(0), ign(ii) { }

rss_parser::~rss_parser() { }

rss_feed rss_parser::parse() {
	rss_feed feed(ch);
	bool skip_parsing = false;

	feed.set_rssurl(my_uri);

	/*
	 * This is a bit messy.
	 *	- http:// and https:// URLs are downloaded and parsed regularly
	 *	- exec: URLs are executed and their output is parsed
	 *	- filter: URLs are downloaded, executed, and their output is parsed
	 *	- query: URLs are ignored
	 */
	mrss_error_t err;
	int my_errno = 0;
	CURLcode ccode = CURLE_OK;
	if (my_uri.substr(0,5) == "http:" || my_uri.substr(0,6) == "https:") {
		mrss_options_t * options = create_mrss_options();
		{
			scope_measure m1("mrss_parse_url_with_options_and_error");
			err = mrss_parse_url_with_options_and_error(const_cast<char *>(my_uri.c_str()), &mrss, options, &ccode);
		}
		my_errno = errno;
		GetLogger().log(LOG_DEBUG, "rss_parser::parse: http URL, err = %u errno = %u (%s)", err, my_errno, strerror(my_errno));
		mrss_options_free(options);
	} else if (my_uri.substr(0,5) == "exec:") {
		std::string file = my_uri.substr(5,my_uri.length()-5);
		std::string buf = utils::get_command_output(file);
		GetLogger().log(LOG_DEBUG, "rss_parser::parse: output of `%s' is: %s", file.c_str(), buf.c_str());
		err = mrss_parse_buffer(const_cast<char *>(buf.c_str()), buf.length(), &mrss);
		my_errno = errno;
	} else if (my_uri.substr(0,7) == "filter:") {
		std::string filter, url;
		utils::extract_filter(my_uri, filter, url);
		std::string buf = utils::retrieve_url(url, utils::get_useragent(cfgcont).c_str());

		char * argv[2];
		argv[0] = const_cast<char *>(filter.c_str());
		argv[1] = NULL;
		std::string result = utils::run_program(argv, buf);
		GetLogger().log(LOG_DEBUG, "rss_parser::parse: output of `%s' is: %s", filter.c_str(), result.c_str());
		err = mrss_parse_buffer(const_cast<char *>(result.c_str()), result.length(), &mrss);
		my_errno = errno;
	} else if (my_uri.substr(0,6) == "query:") {
		skip_parsing = true;
		err = MRSS_OK;
	} else {
		throw utils::strprintf(_("Error: unsupported URL: %s"), my_uri.c_str());
	}

	if (!skip_parsing) {

		if (!mrss)
			return feed;

		if (err > MRSS_OK && err <= MRSS_ERR_DATA) {
			if (err == MRSS_ERR_POSIX) {
				GetLogger().log(LOG_ERROR,"rss_parser::parse: mrss_parse_* failed with POSIX error: error = %s",strerror(my_errno));
			}
			GetLogger().log(LOG_ERROR,"rss_parser::parse: mrss_parse_* failed: err = %s (%u %x)",mrss_strerror(err), err, err);
			GetLogger().log(LOG_ERROR,"rss_parser::parse: CURLcode = %u (%s)", ccode, curl_easy_strerror(ccode));
			GetLogger().log(LOG_DEBUG,"rss_parser::parse: saved errno = %d (%s)", my_errno, strerror(my_errno));
			GetLogger().log(LOG_USERERROR, "RSS feed `%s' couldn't be parsed: %s (error %u)", my_uri.c_str(), mrss_strerror(err), err);
			if (mrss) {
				mrss_free(mrss);
			}
			throw std::string(mrss_strerror(err));
		}

		/*
		 * After parsing is done, we fill our feed object with title,
		 * description, etc.  It's important to note that all data that comes
		 * from mrss must be converted to UTF-8 before, because all data is
		 * internally stored as UTF-8, and converted on-the-fly in case some
		 * other encoding is required. This is because UTF-8 can hold all
		 * available Unicode characters, unlike other non-Unicode encodings.
		 */

		const char * encoding = mrss->encoding ? mrss->encoding : "utf-8";

		if (mrss->title) {
			if (mrss->title_type && (strcmp(mrss->title_type,"xhtml")==0 || strcmp(mrss->title_type,"html")==0)) {
				std::string xhtmltitle = utils::convert_text(mrss->title, "utf-8", encoding);
				feed.set_title(render_xhtml_title(xhtmltitle, feed.link()));
			} else {
				feed.set_title(utils::convert_text(mrss->title, "utf-8", encoding));
			}
		}
		
		if (mrss->description) {
			feed.set_description(utils::convert_text(mrss->description, "utf-8", encoding));
		}

		if (mrss->link) {
			feed.set_link(utils::absolute_url(my_uri, mrss->link));
		}

		if (mrss->pubDate) 
			feed.set_pubDate(parse_date(mrss->pubDate));
		else
			feed.set_pubDate(::time(NULL));

		if (mrss->language) {
			set_rtl(feed, mrss->language);
		}

		GetLogger().log(LOG_DEBUG, "rss_parser::parse: feed title = `%s' link = `%s'", feed.title().c_str(), feed.link().c_str());

		/*
		 * Then we iterate over all items. Each item is filled with title, description,
		 * etc. and is then appended to the feed.
		 */
		for (mrss_item_t * item = mrss->item; item != NULL; item = item->next ) {
			rss_item x(ch);
			if (item->title) {
				if (item->title_type && (strcmp(item->title_type,"xhtml")==0 || strcmp(item->title_type,"html")==0)) {
					std::string xhtmltitle = utils::convert_text(item->title, "utf-8", encoding);
					x.set_title(render_xhtml_title(xhtmltitle, feed.link()));
				} else {
					std::string title = utils::convert_text(item->title, "utf-8", encoding);
					replace_newline_characters(title);
					x.set_title(title);
				}
			}
			if (item->link) {
				x.set_link(utils::absolute_url(my_uri, item->link));
			}
			if (!item->author || strcmp(item->author,"")==0) {
				if (mrss->managingeditor)
					x.set_author(utils::convert_text(mrss->managingeditor, "utf-8", encoding));
				else {
					mrss_tag_t * creator;
					if (mrss_search_tag(item, "creator", "http://purl.org/dc/elements/1.1/", &creator) == MRSS_OK && creator) {
						if (creator->value) {
							x.set_author(utils::convert_text(creator->value, "utf-8", encoding));
						}
					}
				}
			} else {
				x.set_author(utils::convert_text(item->author, "utf-8", encoding));
			}

			x.set_feedurl(feed.rssurl());

			mrss_tag_t * content;

			/*
			 * There are so many different ways in use to transport the "content" or "description".
			 * Why try a number of them to find the content, if possible:
			 * 	- "content:encoded"
			 * 	- Atom's "content"
			 * 	- Apple's "itunes:summary" that can be found in iTunes-compatible podcasts
			 * 	- last but not least, we try the standard description.
			 */
			if (mrss_search_tag(item, "encoded", "http://purl.org/rss/1.0/modules/content/", &content) == MRSS_OK && content) {
				/* RSS 2.0 content:encoded */
				GetLogger().log(LOG_DEBUG, "rss_parser::parse: found content:encoded: %s\n", content->value);
				if (content->value) {
					std::string desc = utils::convert_text(content->value, "utf-8", encoding);
					GetLogger().log(LOG_DEBUG, "rss_parser::parse: converted description `%s' to `%s'", content->value, desc.c_str());
					x.set_description(desc);
				}
			} else {
				GetLogger().log(LOG_DEBUG, "rss_parser::parse: found no content:encoded");
			}

			if ((mrss->version == MRSS_VERSION_ATOM_0_3 || mrss->version == MRSS_VERSION_ATOM_1_0)) {
				int rc;
				if (((rc = mrss_search_tag(item, "content", "http://www.w3.org/2005/Atom", &content)) == MRSS_OK && content) ||
					((rc = mrss_search_tag(item, "content", "http://purl.org/atom/ns#", &content)) == MRSS_OK && content)) {
					GetLogger().log(LOG_DEBUG, "rss_parser::parse: found atom content: %s\n", content ? content->value : "(content = null)");
					if (content && content->value) {
						x.set_description(utils::convert_text(content->value, "utf-8", encoding));
					}
				} else {
					GetLogger().log(LOG_DEBUG, "rss_parser::parse: mrss_search_tag(content) failed with rc = %d content = %p", rc, content);
				}
			} else {
				GetLogger().log(LOG_DEBUG, "rss_parser::parse: not an atom feed");
			}

			/* last resort: search for itunes:summary tag (may be a podcast) */
			if (x.description().length() == 0 && mrss_search_tag(item, "summary", "http://www.itunes.com/dtds/podcast-1.0.dtd", &content) == MRSS_OK && content) {
				GetLogger().log(LOG_DEBUG, "rss_parser::parse: found itunes:summary: %s\n", content->value);
				if (content->value) {
					/*
					 * We put the <ituneshack> tags around the tags so that the HTML renderer
					 * knows that it must not ignore the newlines. It is a really braindead
					 * use of XML to depend on the exact interpretation of whitespaces.
					 */
					std::string desc = "<ituneshack>";
					desc.append(utils::convert_text(content->value, "utf-8", encoding));
					desc.append("</ituneshack>");
					x.set_description(desc);
				}
				
			} else {
				GetLogger().log(LOG_DEBUG, "rss_parser::parse: no luck with itunes:summary");
			}

			if (x.description().length() == 0) {
				if (item->description)
					x.set_description(utils::convert_text(item->description, "utf-8", encoding));
			} else {
				if (cfgcont->get_configvalue_as_bool("always-display-description") && item->description)
					x.set_description(x.description() + "<hr>" + utils::convert_text(item->description, "utf-8", encoding));
			}

			if (item->pubDate) 
				x.set_pubDate(parse_date(item->pubDate));
			else
				x.set_pubDate(::time(NULL));
				
			/*
			 * We try to find a GUID (some unique identifier) for an item. If the regular
			 * GUID is not available (oh, well, there are a few broken feeds around, after
			 * all), we try out the link and the title, instead. This is suboptimal, of course,
			 * because it makes it impossible to recognize duplicates when the title or the
			 * link changes.
			 */
			if (item->guid)
				x.set_guid(item->guid);
			else if (item->link)
				x.set_guid(item->link); // XXX hash something to get a better alternative GUID
			else if (item->title)
				x.set_guid(item->title);
			// ...else?! that's too bad.

			if (item->enclosure_url) {
				x.set_enclosure_url(item->enclosure_url);
				GetLogger().log(LOG_DEBUG, "rss_parser::parse: found enclosure_url: %s", item->enclosure_url);
			}
			if (item->enclosure_type) {
				x.set_enclosure_type(item->enclosure_type);
				GetLogger().log(LOG_DEBUG, "rss_parser::parse: found enclosure_type: %s", item->enclosure_type);
			}

			// x.set_feedptr(&feed);

			GetLogger().log(LOG_DEBUG, "rss_parser::parse: item title = `%s' link = `%s' pubDate = `%s' (%d) description = `%s'", 
				x.title().c_str(), x.link().c_str(), x.pubDate().c_str(), x.pubDate_timestamp(), x.description().c_str());

			// only add item to feed if it isn't on the ignore list or if there is no ignore list
			if (!ign || !ign->matches(&x)) {
				feed.items().push_back(x);
				GetLogger().log(LOG_INFO, "rss_parser::parse: added article title = `%s' link = `%s' ign = %p", x.title().c_str(), x.link().c_str(), ign);
			} else {
				GetLogger().log(LOG_INFO, "rss_parser::parse: ignored article title = `%s' link = `%s'", x.title().c_str(), x.link().c_str());
			}
		}

		feed.remove_old_deleted_items();

		mrss_free(mrss);

	}

	feed.set_empty(false);

	return feed;
}

bool rss_parser::check_and_update_lastmodified() {
	if (my_uri.substr(0,5) != "http:" && my_uri.substr(0,6) != "https:")
		return true;

	if (ign && ign->matches_lastmodified(my_uri)) {
		GetLogger().log(LOG_DEBUG, "rss_parser::check_and_update_lastmodified: found %s on list of URLs that are always downloaded", my_uri.c_str());
		return true;
	}

	time_t oldlm = ch->get_lastmodified(my_uri);
	time_t newlm = 0;
	mrss_error_t err;

	mrss_options_t * options = create_mrss_options();
	err = mrss_get_last_modified_with_options(const_cast<char *>(my_uri.c_str()), &newlm, options);
	mrss_options_free(options);

	GetLogger().log(LOG_DEBUG, "rss_parser::check_and_update_lastmodified: err = %u oldlm = %d newlm = %d", err, oldlm, newlm);

	if (err != MRSS_OK) {
		GetLogger().log(LOG_DEBUG, "rss_parser::check_and_update_lastmodified: no, don't download, due to error");
		return false;
	}

	if (newlm == 0) {
		GetLogger().log(LOG_DEBUG, "rss_parser::check_and_update_lastmodified: yes, download (no Last-Modified header)");
		return true;
	}

	if (newlm > oldlm) {
		ch->set_lastmodified(my_uri, newlm);
		GetLogger().log(LOG_DEBUG, "rss_parser::check_and_update_lastmodified: yes, download");
		return true;
	}

	GetLogger().log(LOG_DEBUG, "rss_parser::check_and_update_lastmodified: no, don't download");
	return false;
}

time_t rss_parser::parse_date(const std::string& datestr) {
	std::istringstream is(datestr);
	std::string monthstr, time, tmp;
	struct tm stm;
	
	memset(&stm,0,sizeof(stm));
	
	is >> tmp;
	if (tmp[tmp.length()-1] == ',')
		is >> tmp;
	
	std::istringstream dayis(tmp);
	dayis >> stm.tm_mday;
	
	is >> monthstr;
	
	stm.tm_mon = monthname_to_number(monthstr);
	
	int year;
	is >> year;
	if (year < 100)
		year += 2000;
	stm.tm_year = year - 1900;
	
	is >> time;

    stm.tm_hour = stm.tm_min = stm.tm_sec = 0;
	
	std::vector<std::string> tkns = utils::tokenize(time,":");
	if (tkns.size() > 0) {
		std::istringstream hs(tkns[0]);
		hs >> stm.tm_hour;
		if (tkns.size() > 1) {
			std::istringstream ms(tkns[1]);
			ms >> stm.tm_min;
			if (tkns.size() > 2) {
				std::istringstream ss(tkns[2]);
				ss >> stm.tm_sec;
			}
		}
	}

	time_t value = mktime(&stm);
	return value;
}

void rss_parser::replace_newline_characters(std::string& str) {
	str = utils::replace_all(str, "\r", " ");
	str = utils::replace_all(str, "\n", " ");
}

mrss_options_t * rss_parser::create_mrss_options() {
	char * proxy = NULL;
	char * proxy_auth = NULL;

	if (cfgcont->get_configvalue_as_bool("use-proxy") == true) {
		proxy = const_cast<char *>(cfgcont->get_configvalue("proxy").c_str());
		proxy_auth = const_cast<char *>(cfgcont->get_configvalue("proxy-auth").c_str());
	}

	return mrss_options_new(30, proxy, proxy_auth, NULL, NULL, NULL, 0, NULL, utils::get_useragent(cfgcont).c_str());
}

std::string rss_parser::render_xhtml_title(const std::string& title, const std::string& link) {
	htmlrenderer rnd(1 << 16); // a huge number
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

void rss_parser::set_rtl(rss_feed& feed, const char * lang) {
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
			GetLogger().log(LOG_DEBUG, "rss_parser::parse: detected right-to-left order, language code = %s", rtl_langprefix[i]);
			feed.set_rtl(true);
			break;
		}
	}
}

}

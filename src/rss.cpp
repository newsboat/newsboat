#include <rss.h>
#include <config.h>
#include <cache.h>
#include <xmlpullparser.h>
#include <utils.h>
#include <logger.h>
#include <sstream>
#include <iostream>
#include <configcontainer.h>
#include <curl/curl.h>
#include <sys/utsname.h>

#include <langinfo.h>

using namespace newsbeuter;

rss_parser::rss_parser(const char * uri, cache * c, configcontainer * cfg) : my_uri(uri), ch(c), cfgcont(cfg), mrss(0) { }

rss_parser::~rss_parser() { }

rss_feed rss_parser::parse() {
	rss_feed feed(ch);

	feed.set_rssurl(my_uri);

	char * proxy = NULL;
	char * proxy_auth = NULL;

	if (cfgcont->get_configvalue_as_bool("use-proxy") == true) {
		proxy = const_cast<char *>(cfgcont->get_configvalue("proxy").c_str());
		proxy_auth = const_cast<char *>(cfgcont->get_configvalue("proxy-auth").c_str());
	}

	char user_agent[1024];
	std::string ua_pref = cfgcont->get_configvalue("user-agent");
	if (ua_pref.length() == 0) {
		struct utsname buf;
		uname(&buf);
		snprintf(user_agent, sizeof(user_agent), "%s/%s (%s %s; %s; %s) %s", PROGRAM_NAME, PROGRAM_VERSION, buf.sysname, buf.release, buf.machine, PROGRAM_URL, curl_version());
	} else {
		snprintf(user_agent, sizeof(user_agent), "%s", ua_pref.c_str());
	}

	mrss_error_t err;
	if (my_uri.substr(0,5) == "http:" || my_uri.substr(0,6) == "https:") {
		mrss_options_t * options = mrss_options_new(30, proxy, proxy_auth, NULL, NULL, NULL, 0, NULL, user_agent);
		err = mrss_parse_url_with_options(const_cast<char *>(my_uri.c_str()), &mrss, options);
		mrss_options_free(options);
	} else if (my_uri.substr(0,5) == "exec:") {
		std::string file = my_uri.substr(5,my_uri.length()-5);
		std::string buf = utils::get_command_output(file);
		GetLogger().log(LOG_DEBUG, "rss_parser::parse: output of `%s' is: %s", file.c_str(), buf.c_str());
		err = mrss_parse_buffer(const_cast<char *>(buf.c_str()), buf.length(), &mrss);
	} else if (my_uri.substr(0,7) == "filter:") {
		std::string filter, url;
		utils::extract_filter(my_uri, filter, url);
		std::string buf = utils::retrieve_url(url, user_agent);
		std::string result = utils::run_filter(filter, buf);
		GetLogger().log(LOG_DEBUG, "rss_parser::parse: output of `%s' is: %s", filter.c_str(), result.c_str());
		err = mrss_parse_buffer(const_cast<char *>(result.c_str()), result.length(), &mrss);
	} else {
		char buf[1024];
		snprintf(buf, sizeof(buf), _("Error: unsupported URL: %s"), my_uri.c_str());
		throw std::string(buf);
	}

	if (err != MRSS_OK) {
		GetLogger().log(LOG_ERROR,"rss_parser::parse: mrss_parse_url_with_options failed: err = %s (%d)",mrss_strerror(err), err);
		if (mrss) {
			mrss_free(mrss);
		}
		throw std::string(mrss_strerror(err));
	}

	const char * encoding = mrss->encoding ? mrss->encoding : "utf-8";

	if (mrss->title) {
		feed.set_title(utils::convert_text(mrss->title, "utf-8", encoding));
	}
	
	if (mrss->description) {
		feed.set_description(utils::convert_text(mrss->description, "utf-8", encoding));
	}

	if (mrss->link) feed.set_link(mrss->link);
	if (mrss->pubDate) 
		feed.set_pubDate(parse_date(mrss->pubDate));
	else
		feed.set_pubDate(::time(NULL));

	GetLogger().log(LOG_DEBUG, "rss_parser::parse: feed title = `%s' link = `%s'", feed.title().c_str(), feed.link().c_str());

	for (mrss_item_t * item = mrss->item; item != NULL; item = item->next ) {
		rss_item x(ch);
		if (item->title) {
			std::string title = utils::convert_text(item->title, "utf-8", encoding);
			x.set_title(title);
			GetLogger().log(LOG_DEBUG, "rss_parser::parse: converted title `%s' to `%s'", item->title, title.c_str());
		}
		if (item->link) x.set_link(item->link);
		if (item->author) {
			x.set_author(utils::convert_text(item->author, "utf-8", encoding));
		}

		mrss_tag_t * content;

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
				std::string desc = "<ituneshack>";
				desc.append(utils::convert_text(content->value, "utf-8", encoding));
				desc.append("</ituneshack>");
				x.set_description(desc);
			}
			
		} else {
			GetLogger().log(LOG_DEBUG, "rss_parser::parse: no luck with itunes:summary");
		}

		if (x.description().length() == 0 && item->description) {
			x.set_description(utils::convert_text(item->description, "utf-8", encoding));
		}

		if (item->pubDate) 
			x.set_pubDate(parse_date(item->pubDate));
		else
			x.set_pubDate(::time(NULL));
			
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

		GetLogger().log(LOG_DEBUG, "rss_parser::parse: item title = `%s' link = `%s' pubDate = `%s' (%d) description = `%s'", 
			x.title().c_str(), x.link().c_str(), x.pubDate().c_str(), x.pubDate_timestamp(), x.description().c_str());
		feed.items().push_back(x);
	}

	mrss_free(mrss);

	return feed;
}

// rss_item setters

void rss_item::set_title(const std::string& t) { 
	title_ = t; 
}


void rss_item::set_link(const std::string& l) { 
	link_ = l; 
}

void rss_item::set_author(const std::string& a) { 
	author_ = a; 
}

void rss_item::set_description(const std::string& d) { 
	description_ = d; 
}

void rss_item::set_pubDate(time_t t) { 
	pubDate_ = t; 
}

void rss_item::set_guid(const std::string& g) { 
	guid_ = g; 
}

void rss_item::set_unread_nowrite(bool u) {
	unread_ = u;
}

void rss_item::set_unread(bool u) { 
	if (unread_ != u) {
		unread_ = u;
		if (ch) ch->update_rssitem_unread_and_enqueued(*this, feedurl_); 
	}
}

std::string rss_item::pubDate() const {
	char text[1024];
	strftime(text,sizeof(text),"%a, %d %b %Y %T", gmtime(&pubDate_)); 
	return std::string(text);
}

unsigned int rss_feed::unread_item_count() const {
	unsigned int count = 0;
	for (std::vector<rss_item>::const_iterator it=items_.begin();it!=items_.end();++it) {
		if (it->unread())
			++count;
	}
	return count;
}

time_t rss_parser::parse_date(const std::string& datestr) {
	// TODO: refactor	
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
	
	if (monthstr == "Jan")
		stm.tm_mon = 0;
	else if (monthstr == "Feb")
		stm.tm_mon = 1;
	else if (monthstr == "Mar")
		stm.tm_mon = 2;
	else if (monthstr == "Apr")
		stm.tm_mon = 3;
	else if (monthstr == "May")
		stm.tm_mon = 4;
	else if (monthstr == "Jun")
		stm.tm_mon = 5;
	else if (monthstr == "Jul")
		stm.tm_mon = 6;
	else if (monthstr == "Aug")
		stm.tm_mon = 7;
	else if (monthstr == "Sep")
		stm.tm_mon = 8;
	else if (monthstr == "Oct")
		stm.tm_mon = 9;
	else if (monthstr == "Nov")
		stm.tm_mon = 10;
	else if (monthstr == "Dec")
		stm.tm_mon = 11;
	
	int year;
	is >> year;
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

bool rss_feed::matches_tag(const std::string& tag) {
	for (std::vector<std::string>::iterator it=tags_.begin();it!=tags_.end();++it) {
		if (tag == *it)
			return true;
	}
	return false;
}

std::string rss_feed::get_tags() {
	std::string tags;
	for (std::vector<std::string>::iterator it=tags_.begin();it!=tags_.end();++it) {
		tags.append(*it);
		tags.append(" ");
	}
	return tags;
}

void rss_feed::set_tags(const std::vector<std::string>& tags) {
	if (tags_.size() > 0)
		tags_.erase(tags_.begin(), tags_.end());
	for (std::vector<std::string>::const_iterator it=tags.begin();it!=tags.end();++it) {
		tags_.push_back(*it);
	}
}

void rss_item::set_enclosure_url(const std::string& url) {
	enclosure_url_ = url;
}

void rss_item::set_enclosure_type(const std::string& type) {
	enclosure_type_ = type;
}

std::string rss_item::title() const {
	GetLogger().log(LOG_DEBUG,"rss_item::title: title before conversion: %s", title_.c_str());
	std::string retval = utils::convert_text(title_, nl_langinfo(CODESET), "utf-8");
	GetLogger().log(LOG_DEBUG,"rss_item::title: title after conversion: %s", retval.c_str());
	return retval;
}

std::string rss_item::author() const {
	return utils::convert_text(author_, nl_langinfo(CODESET), "utf-8");
}

std::string rss_item::description() const {
	return utils::convert_text(description_, nl_langinfo(CODESET), "utf-8");
}

std::string rss_feed::title() const {
	return utils::convert_text(title_, nl_langinfo(CODESET), "utf-8");
}

std::string rss_feed::description() const {
	return utils::convert_text(description_, nl_langinfo(CODESET), "utf-8");
}

rss_item& rss_feed::get_item_by_guid(const std::string& guid) {
	for (std::vector<rss_item>::iterator it=items_.begin();it!=items_.end();++it) {
		if (it->guid() == guid)
			return *it;
	}
	static rss_item dummy_item(0); // should never happen!
	return dummy_item;
}

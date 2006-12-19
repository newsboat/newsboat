#include <rss.h>
#include <config.h>
#include <stringprep.h>
#include <cache.h>
#include <xmlpullparser.h>
#include <utils.h>
#include <sstream>
#include <iostream>

using namespace noos;

rss_parser::rss_parser(const char * uri, cache * c) : my_uri(uri), ch(c), mrss(0) { }

rss_parser::~rss_parser() { }

rss_feed rss_parser::parse() {
	rss_feed feed(ch);

	feed.set_rssurl(my_uri);

	mrss_options_t * options = mrss_options_new(-1, NULL, NULL, NULL, NULL, 0, NULL, USER_AGENT);
	mrss_error_t err = mrss_parse_url_with_options(const_cast<char *>(my_uri.c_str()), &mrss, options);
	mrss_options_free(options);

	if (err != MRSS_OK) {
		// TODO: throw exception
		if (mrss) {
			mrss_free(mrss);
		}
		return feed;
	}

	if (mrss->title) feed.set_title(mrss->title);
	if (mrss->description) feed.set_description(mrss->description);
	if (mrss->link) feed.set_link(mrss->link);
	if (mrss->pubDate) 
		feed.set_pubDate(parse_date(mrss->pubDate));
	else
		feed.set_pubDate(::time(NULL));

	for (mrss_item_t * item = mrss->item; item != NULL; item = item->next ) {
		rss_item x(ch);
		if (item->title) {
			char * str = stringprep_convert(item->title,stringprep_locale_charset(),mrss->encoding);
			if (str) {
				x.set_title(str);
				free(str);
			}
		}
		if (item->link) x.set_link(item->link);
		if (item->author) x.set_author(item->author);
		if (item->description) {
			char * str = stringprep_convert(item->description,stringprep_locale_charset(),mrss->encoding);
			if (str) {
				x.set_description(str);
				free(str);
			}
		}
		if (item->pubDate) 
			x.set_pubDate(parse_date(item->pubDate));
		else
			x.set_pubDate(::time(NULL));
			
		if (item->guid)
			x.set_guid(item->guid);
		else
			x.set_guid(item->link); // XXX hash something to get a better alternative GUID
		// x.set_dirty();
		feed.items().push_back(x);
	}

	mrss_free(mrss);

	return feed;
}

// rss_item setters

void rss_item::set_title(const std::string& t) { 
	title_ = t; 
	// if (ch) ch->update_rssitem(*this, feedurl);
}


void rss_item::set_link(const std::string& l) { 
	link_ = l; 
	// if (ch) ch->update_rssitem(*this, feedurl);	
}

void rss_item::set_author(const std::string& a) { 
	author_ = a; 
	// if (ch) ch->update_rssitem(*this, feedurl);
}

void rss_item::set_description(const std::string& d) { 
	description_ = d; 
	// if (ch) ch->update_rssitem(*this, feedurl);
}

void rss_item::set_pubDate(time_t t) { 
	pubDate_ = t; 
	// if (ch) ch->update_rssitem(*this, feedurl);
}

void rss_item::set_guid(const std::string& g) { 
	guid_ = g; 
	// if (ch) ch->update_rssitem(*this, feedurl);
}

void rss_item::set_unread(bool u) { 
	if (unread_ != u) {
		unread_ = u;
		if (ch) ch->update_rssitem_unread(*this, feedurl_); 
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
	
	std::vector<std::string> tkns = utils::tokenize(time,":");
	std::istringstream hs(tkns[0]);
	hs >> stm.tm_hour;
	std::istringstream ms(tkns[1]);
	ms >> stm.tm_min;
	std::istringstream ss(tkns[2]);
	ss >> stm.tm_sec;
	
	time_t value = mktime(&stm);
	return value;
}


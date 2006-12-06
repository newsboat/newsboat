#include <rss.h>
#include <config.h>
#include <stringprep.h>
#include <cache.h>

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
	if (mrss->pubDate) feed.set_pubDate(mrss->pubDate);

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
		if (item->pubDate) x.set_pubDate(item->pubDate);
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

void rss_item::set_pubDate(const std::string& d) { 
	pubDate_ = d; 
	// if (ch) ch->update_rssitem(*this, feedurl);
}

void rss_item::set_guid(const std::string& g) { 
	guid_ = g; 
	// if (ch) ch->update_rssitem(*this, feedurl);
}

void rss_item::set_unread(bool u) { 
	unread_ = u;
	if (ch) ch->update_rssitem_unread(*this, feedurl); 
}


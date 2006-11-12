#include <rss.h>

using namespace noos;

rss_parser::rss_parser(const char * uri) : my_uri(uri), mrss(0) { }

rss_parser::~rss_parser() { }

void rss_parser::parse() {
	mrss_error_t err = mrss_parse_url(const_cast<char *>(my_uri.c_str()), &mrss);
	if (err != MRSS_OK) {
		// TODO: throw exception
		if (mrss) {
			mrss_free(mrss);
		}
		return;
	}

	if (feed.items().size() > 0) {
		feed.items().erase(feed.items().begin(),feed.items().end());
	}

	feed.rssurl() = my_uri;

	if (mrss->title) feed.title() = mrss->title;
	if (mrss->description) feed.description() = mrss->description;
	if (mrss->link) feed.link() = mrss->link;
	if (mrss->pubDate) feed.pubDate() = mrss->pubDate;

	for (mrss_item_t * item = mrss->item; item != NULL; item = item->next ) {
		rss_item x;
		if (item->title) x.title() = item->title;
		if (item->link) x.link() = item->link;
		if (item->author) x.author() = item->author;
		if (item->description) x.description() = item->description;
		if (item->pubDate) x.pubDate() = item->pubDate;
		if (item->guid) 
			x.guid() = item->guid;
		else
			x.guid() = item->link; // XXX hash something to get a better alternative GUID
		feed.items().push_back(x);
	}

	mrss_free(mrss);
}

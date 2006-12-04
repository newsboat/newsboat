#include <rss.h>
#include <config.h>
#include <stringprep.h>

using namespace noos;

rss_parser::rss_parser(const char * uri) : my_uri(uri), mrss(0) { }

rss_parser::~rss_parser() { }

rss_feed rss_parser::parse() {
	rss_feed feed;

	feed.rssurl() = my_uri;

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

	if (mrss->title) feed.title() = mrss->title;
	if (mrss->description) feed.description() = mrss->description;
	if (mrss->link) feed.link() = mrss->link;
	if (mrss->pubDate) feed.pubDate() = mrss->pubDate;

	for (mrss_item_t * item = mrss->item; item != NULL; item = item->next ) {
		rss_item x;
		if (item->title) {
			char * str = stringprep_convert(item->title,stringprep_locale_charset(),mrss->encoding);
			if (str) {
				x.title() = str;
				free(str);
			}
		}
		if (item->link) x.link() = item->link;
		if (item->author) x.author() = item->author;
		if (item->description) {
			char * str = stringprep_convert(item->description,stringprep_locale_charset(),mrss->encoding);
			if (str) {
				x.description() = str;
				free(str);
			}
		}
		if (item->pubDate) x.pubDate() = item->pubDate;
		if (item->guid)
			x.guid() = item->guid;
		else
			x.guid() = item->link; // XXX hash something to get a better alternative GUID
		x.set_dirty();
		feed.items().push_back(x);
	}

	mrss_free(mrss);

	return feed;
}

#ifndef NEWSBOAT_RSSPARSER_H_
#define NEWSBOAT_RSSPARSER_H_

#include <memory>
#include <string>

#include "remoteapi.h"
#include "rss/feed.h"

namespace rsspp {
class Item;
}

namespace Newsboat {

class Cache;
class ConfigContainer;
class RssFeed;
class RssIgnores;
class RssItem;

class RssParser {
public:
	RssParser(const std::string& uri,
		Cache& c,
		ConfigContainer&,
		RssIgnores* ii);
	~RssParser();
	std::shared_ptr<RssFeed> parse(const rsspp::Feed& upstream_feed);
	bool check_and_update_lastmodified();

private:
	void replace_newline_characters(std::string& str);
	std::string render_xhtml_title(const std::string& title,
		const std::string& link);
	time_t parse_date(const std::string& datestr);
	void set_rtl(std::shared_ptr<RssFeed> feed, const std::string& lang);

	void fill_feed_fields(std::shared_ptr<RssFeed> feed, const rsspp::Feed& upstream_feed);
	void fill_feed_items(std::shared_ptr<RssFeed> feed, const rsspp::Feed& upstream_feed);

	void set_item_title(std::shared_ptr<RssFeed> feed,
		std::shared_ptr<RssItem> x,
		const rsspp::Item& item);
	void set_item_author(std::shared_ptr<RssItem> x,
		const rsspp::Item& item, const rsspp::Feed& upstream_feed);
	void set_item_content(std::shared_ptr<RssItem> x,
		const rsspp::Item& item);
	void set_item_enclosure(std::shared_ptr<RssItem> x,
		const rsspp::Item& item);
	std::string get_guid(const rsspp::Item& item) const;

	void add_item_to_feed(std::shared_ptr<RssFeed> feed,
		std::shared_ptr<RssItem> item);

	void handle_content_encoded(std::shared_ptr<RssItem> x,
		const rsspp::Item& item) const;
	void handle_itunes_summary(std::shared_ptr<RssItem> x,
		const rsspp::Item& item);
	bool is_html_type(const std::string& type);

	std::string my_uri;
	Cache& ch;
	ConfigContainer& cfgcont;
	RssIgnores* ign;
};

} // namespace Newsboat

#endif /* NEWSBOAT_RSSPARSER_H_ */

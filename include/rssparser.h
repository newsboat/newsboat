#ifndef NEWSBOAT_RSSPARSER_H_
#define NEWSBOAT_RSSPARSER_H_

#include <memory>
#include <string>

#include "remoteapi.h"
#include "rss/feed.h"

namespace rsspp {
class Item;
}

namespace newsboat {

class Cache;
class ConfigContainer;
class CurlHandle;
class RssFeed;
class RssIgnores;
class RssItem;

class RssParser {
public:
	RssParser(const std::string& uri,
		Cache* c,
		ConfigContainer*,
		RssIgnores* ii,
		RemoteApi* a = 0);
	~RssParser();
	std::shared_ptr<RssFeed> parse();
	bool check_and_update_lastmodified();

	void set_easyhandle(CurlHandle* h)
	{
		easyhandle = h;
	}

private:
	void replace_newline_characters(std::string& str);
	std::string render_xhtml_title(const std::string& title,
		const std::string& link);
	time_t parse_date(const std::string& datestr);
	void set_rtl(std::shared_ptr<RssFeed> feed, const std::string& lang);

	void retrieve_uri(const std::string& uri);
	void download_http(const std::string& uri);
	void get_execplugin(const std::string& plugin);
	void download_filterplugin(const std::string& filter,
		const std::string& uri);
	void parse_file(const std::string& file);

	void fill_feed_fields(std::shared_ptr<RssFeed> feed);
	void fill_feed_items(std::shared_ptr<RssFeed> feed);

	void set_item_title(std::shared_ptr<RssFeed> feed,
		std::shared_ptr<RssItem> x,
		const rsspp::Item& item);
	void set_item_author(std::shared_ptr<RssItem> x,
		const rsspp::Item& item);
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
	void fetch_ttrss(const std::string& feed_id);
	void fetch_newsblur(const std::string& feed_id);
	void fetch_ocnews(const std::string& feed_id);
	void fetch_feedly(const std::string& feed_id);

	std::string my_uri;
	Cache* ch;
	ConfigContainer* cfgcont;
	RssIgnores* ign;
	rsspp::Feed f;
	RemoteApi* api;
	bool is_ttrss;
	bool is_newsblur;
	bool is_ocnews;
	bool is_feedly;

	CurlHandle* easyhandle;
};

} // namespace newsboat

#endif /* NEWSBOAT_RSSPARSER_H_ */

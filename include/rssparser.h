#ifndef NEWSBOAT_RSSPARSER_H_
#define NEWSBOAT_RSSPARSER_H_

#include <string>

#include "remoteapi.h"
#include "rss.h"
#include "rsspp.h"

namespace newsboat {

class ConfigContainer;
class Cache;

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
	void Download_http(const std::string& uri);
	void get_execplugin(const std::string& plugin);
	void Download_filterplugin(const std::string& filter,
		const std::string& uri);
	void parse_file(const std::string& file);

	void fill_feed_fields(std::shared_ptr<RssFeed> feed);
	void fill_feed_items(std::shared_ptr<RssFeed> feed);

	void set_item_title(std::shared_ptr<RssFeed> feed,
		std::shared_ptr<RssItem> x,
		const rsspp::item& item);
	void set_item_author(std::shared_ptr<RssItem> x,
		const rsspp::item& item);
	void set_item_content(std::shared_ptr<RssItem> x,
		const rsspp::item& item);
	void set_item_enclosure(std::shared_ptr<RssItem> x,
		const rsspp::item& item);
	std::string get_guid(const rsspp::item& item) const;

	void add_item_to_feed(std::shared_ptr<RssFeed> feed,
		std::shared_ptr<RssItem> item);

	void handle_content_encoded(std::shared_ptr<RssItem> x,
		const rsspp::item& item) const;
	void handle_itunes_summary(std::shared_ptr<RssItem> x,
		const rsspp::item& item);
	bool is_html_type(const std::string& type);
	void fetch_ttrss(const std::string& feed_id);
	void fetch_NewsBlur(const std::string& feed_id);
	void fetch_ocnews(const std::string& feed_id);

	std::string my_uri;
	Cache* ch;
	ConfigContainer* cfgcont;
	bool skip_parsing;
	bool is_valid;
	RssIgnores* ign;
	rsspp::feed f;
	RemoteApi* api;
	bool is_ttrss;
	bool is_NewsBlur;
	bool is_ocnews;

	CurlHandle* easyhandle;
};

} // namespace newsboat

#endif /* NEWSBOAT_RSSPARSER_H_ */

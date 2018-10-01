#ifndef NEWSBOAT_RSSPARSER_H_
#define NEWSBOAT_RSSPARSER_H_

#include <string>

#include "remoteapi.h"
#include "rss.h"
#include "rsspp.h"

namespace newsboat {

class configcontainer;
class cache;

class rss_parser {
public:
	rss_parser(const std::string& uri,
		cache* c,
		configcontainer*,
		rss_ignores* ii,
		remote_api* a = 0);
	~rss_parser();
	std::shared_ptr<rss_feed> parse();
	bool check_and_update_lastmodified();

	void set_easyhandle(curl_handle* h)
	{
		easyhandle = h;
	}

private:
	void replace_newline_characters(std::string& str);
	std::string render_xhtml_title(const std::string& title,
		const std::string& link);
	time_t parse_date(const std::string& datestr);
	void set_rtl(std::shared_ptr<rss_feed> feed, const std::string& lang);

	void retrieve_uri(const std::string& uri);
	void download_http(const std::string& uri);
	void get_execplugin(const std::string& plugin);
	void download_filterplugin(const std::string& filter,
		const std::string& uri);
	void parse_file(const std::string& file);

	void fill_feed_fields(std::shared_ptr<rss_feed> feed);
	void fill_feed_items(std::shared_ptr<rss_feed> feed);

	void set_item_title(std::shared_ptr<rss_feed> feed,
		std::shared_ptr<rss_item> x,
		const rsspp::item& item);
	void set_item_author(std::shared_ptr<rss_item> x,
		const rsspp::item& item);
	void set_item_content(std::shared_ptr<rss_item> x,
		const rsspp::item& item);
	void set_item_enclosure(std::shared_ptr<rss_item> x,
		const rsspp::item& item);
	std::string get_guid(const rsspp::item& item) const;

	void add_item_to_feed(std::shared_ptr<rss_feed> feed,
		std::shared_ptr<rss_item> item);

	void handle_content_encoded(std::shared_ptr<rss_item> x,
		const rsspp::item& item) const;
	void handle_itunes_summary(std::shared_ptr<rss_item> x,
		const rsspp::item& item);
	bool is_html_type(const std::string& type);
	void fetch_ttrss(const std::string& feed_id);
	void fetch_newsblur(const std::string& feed_id);
	void fetch_ocnews(const std::string& feed_id);

	std::string my_uri;
	cache* ch;
	configcontainer* cfgcont;
	bool skip_parsing;
	bool is_valid;
	rss_ignores* ign;
	rsspp::feed f;
	remote_api* api;
	bool is_ttrss;
	bool is_newsblur;
	bool is_ocnews;

	curl_handle* easyhandle;
};

} // namespace newsboat

#endif /* NEWSBOAT_RSSPARSER_H_ */

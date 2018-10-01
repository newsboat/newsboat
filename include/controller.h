#ifndef NEWSBOAT_CONTROLLER_H_
#define NEWSBOAT_CONTROLLER_H_

#include <libxml/tree.h>

#include "cache.h"
#include "colormanager.h"
#include "configcontainer.h"
#include "configpaths.h"
#include "feedcontainer.h"
#include "filtercontainer.h"
#include "fslock.h"
#include "regexmanager.h"
#include "reloader.h"
#include "remoteapi.h"
#include "rss.h"
#include "urlreader.h"
#include "opml.h"

namespace newsboat {

class View;

class CurlHandle;

class Controller {
public:
	Controller();
	~Controller();
	void set_view(View* vv);
	View* get_View()
	{
		return v;
	}
	int run(const CliArgsParser& args);

	std::vector<std::shared_ptr<RssItem>> search_for_items(
		const std::string& query,
		std::shared_ptr<RssFeed> feed);

	void update_feedlist();
	void update_visible_feeds();
	void mark_all_read(unsigned int pos);
	void mark_article_read(const std::string& guid, bool read);
	void mark_all_read(const std::string& feedurl);
	void mark_all_read(std::shared_ptr<RssFeed> feed)
	{
		rsscache->mark_all_read(feed);
	}
	bool get_refresh_on_start() const
	{
		return refresh_on_start;
	}
	void enqueue_url(const std::string& url,
		const std::string& title,
		const time_t pubDate,
		std::shared_ptr<RssFeed> feed);

	void reload_urls_file();
	void edit_urls_file();

	FilterContainer& get_filters()
	{
		return filters;
	}

	Cache* get_cache()
	{
		return rsscache;
	}

	ConfigContainer* get_cfg()
	{
		return &cfg;
	}

	FeedContainer* get_feedcontainer()
	{
		return &feedcontainer;
	}

	void write_item(std::shared_ptr<RssItem> item,
		const std::string& filename);
	void write_item(std::shared_ptr<RssItem> item, std::ostream& ostr);
	std::string write_temporary_item(std::shared_ptr<RssItem> item);

	void update_config();

	void load_configfile(const std::string& filename);

	void dump_config(const std::string& filename);

	void update_flags(std::shared_ptr<RssItem> item);

	Reloader* get_reloader()
	{
		return &reloader;
	}

	void replace_feed(std::shared_ptr<RssFeed> oldfeed,
		std::shared_ptr<RssFeed> newfeed,
		unsigned int pos,
		bool unattended);

	RssIgnores* get_ignores()
	{
		return &ign;
	}

	RemoteApi* get_api()
	{
		return api;
	}

private:
	void import_opml(const std::string& filename);
	void export_opml();
	void rec_find_rss_outlines(xmlNode* node, std::string tag);
	int execute_commands(const std::vector<std::string>& cmds);

	void enqueue_items(std::shared_ptr<RssFeed> feed);

	std::string generate_enqueue_filename(const std::string& url,
		const std::string& title,
		const time_t pubDate,
		std::shared_ptr<RssFeed> feed);
	std::string get_hostname_from_url(const std::string& url);

	void import_read_information(const std::string& readinfofile);
	void export_read_information(const std::string& readinfofile);

	View* v;
	UrlReader* urlcfg;
	Cache* rsscache;
	bool refresh_on_start;
	ConfigContainer cfg;
	RssIgnores ign;
	FeedContainer feedcontainer;
	FilterContainer filters;

	ConfigParser cfgparser;
	ColorManager colorman;
	RegexManager rxman;
	RemoteApi* api;
	std::mutex feeds_mutex;

	std::unique_ptr<FSLock> fslock;

	ConfigPaths configpaths;

	Reloader reloader;
};

} // namespace newsboat

#endif /* NEWSBOAT_CONTROLLER_H_ */

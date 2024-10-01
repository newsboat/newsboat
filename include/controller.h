#ifndef NEWSBOAT_CONTROLLER_H_
#define NEWSBOAT_CONTROLLER_H_

#include <libxml/tree.h>

#include "cache.h"
#include "colormanager.h"
#include "configcontainer.h"
#include "configparser.h"
#include "feedcontainer.h"
#include "filtercontainer.h"
#include "fslock.h"
#include "queuemanager.h"
#include "regexmanager.h"
#include "reloader.h"
#include "remoteapi.h"
#include "rssignores.h"
#include "urlreader.h"

namespace newsboat {

class CliArgsParser;
class ConfigPaths;
class View;

class Controller {
public:
	Controller(ConfigPaths& configpaths);
	~Controller();
	void set_view(View* vv);
	View* get_view()
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
	void mark_all_read(const std::vector<std::string>& item_guids);
	bool get_refresh_on_start() const
	{
		return refresh_on_start;
	}
	EnqueueResult enqueue_url(std::shared_ptr<RssItem> item,
		std::shared_ptr<RssFeed> feed);

	void reload_urls_file();
	void edit_urls_file();

	FeedContainer* get_feedcontainer()
	{
		return &feedcontainer;
	}

	void write_item(std::shared_ptr<RssItem> item,
		const Filepath& filename);
	void write_item(std::shared_ptr<RssItem> item, std::ostream& ostr);
	Filepath write_temporary_item(std::shared_ptr<RssItem> item);

	void update_config();

	void load_configfile(const Filepath& filename);

	void dump_config(const Filepath& filename) const;

	void update_flags(std::shared_ptr<RssItem> item);

	Reloader* get_reloader()
	{
		return reloader.get();
	}

	void replace_feed(std::shared_ptr<RssFeed> oldfeed,
		std::shared_ptr<RssFeed> newfeed,
		unsigned int pos,
		bool unattended);

	ConfigContainer* get_config()
	{
		return &cfg;
	}

	RssIgnores* get_ignores()
	{
		return &ign;
	}

	RemoteApi* get_api()
	{
		return api;
	}

	RegexManager& get_regexmanager()
	{
		return rxman;
	}

	FilterContainer& get_filtercontainer()
	{
		return filters;
	}

	const ColorManager& get_colormanager()
	{
		return colorman;
	}

private:
	int import_opml(const Filepath& opmlFile, const Filepath& urlFile);
	void export_opml(bool version2);
	void rec_find_rss_outlines(xmlNode* node, std::string tag);
	int execute_commands(const std::vector<std::string>& cmds);

	void import_read_information(const Filepath& readinfofile);
	void export_read_information(const Filepath& readinfofile);

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

	FsLock fslock;

	ConfigPaths& configpaths;

	std::unique_ptr<Reloader> reloader;

	QueueManager queueManager;
};

} // namespace newsboat

#endif /* NEWSBOAT_CONTROLLER_H_ */

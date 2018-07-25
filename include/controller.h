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
#include "remote_api.h"
#include "rss.h"
#include "urlreader.h"

namespace newsboat {

extern int ctrl_c_hit;

class view;

class curl_handle;

class controller {
public:
	controller();
	~controller();
	void set_view(view* vv);
	view* get_view()
	{
		return v;
	}
	int run(int argc = 0, char* argv[] = nullptr);

	void reload(unsigned int pos,
		unsigned int max = 0,
		bool unattended = false,
		curl_handle* easyhandle = 0);

	void reload_all(bool unattended = false);
	void reload_indexes(const std::vector<int>& indexes,
		bool unattended = false);
	void reload_range(unsigned int start,
		unsigned int end,
		unsigned int size,
		bool unattended = false);
	void start_reload_all_thread(std::vector<int>* indexes = 0);

	std::vector<std::shared_ptr<rss_item>> search_for_items(
		const std::string& query,
		std::shared_ptr<rss_feed> feed);

	void unlock_reload_mutex()
	{
		reload_mutex.unlock();
	}
	bool trylock_reload_mutex();

	void update_feedlist();
	void update_visible_feeds();
	void mark_all_read(unsigned int pos);
	void mark_article_read(const std::string& guid, bool read);
	void mark_all_read(const std::string& feedurl);
	void mark_all_read(std::shared_ptr<rss_feed> feed)
	{
		rsscache->mark_all_read(feed);
	}
	bool get_refresh_on_start() const
	{
		return refresh_on_start;
	}
	void enqueue_url(const std::string& url,
		std::shared_ptr<rss_feed> feed);
	void notify(const std::string& msg);

	void reload_urls_file();
	void edit_urls_file();

	filtercontainer& get_filters()
	{
		return filters;
	}

	std::string bookmark(const std::string& url,
		const std::string& title,
		const std::string& description,
		const std::string& feed_title);

	cache* get_cache()
	{
		return rsscache;
	}

	configcontainer* get_cfg()
	{
		return &cfg;
	}

	FeedContainer* get_feedcontainer()
	{
		return &feedcontainer;
	}

	void write_item(std::shared_ptr<rss_item> item,
		const std::string& filename);
	void write_item(std::shared_ptr<rss_item> item, std::ostream& ostr);
	std::string write_temporary_item(std::shared_ptr<rss_item> item);

	void mark_deleted(const std::string& guid, bool b);

	void update_config();

	void load_configfile(const std::string& filename);

	void dump_config(const std::string& filename);

	void update_flags(std::shared_ptr<rss_item> item);

private:
	void print_usage(char* argv0);
	void print_version_information(const char* argv0, unsigned int level);
	void import_opml(const std::string& filename);
	void export_opml();
	void rec_find_rss_outlines(xmlNode* node, std::string tag);
	int execute_commands(const std::vector<std::string>& cmds);

	std::string prepare_message(unsigned int pos, unsigned int max);
	void save_feed(std::shared_ptr<rss_feed> feed, unsigned int pos);
	void enqueue_items(std::shared_ptr<rss_feed> feed);

	std::string generate_enqueue_filename(const std::string& url,
		std::shared_ptr<rss_feed> feed);
	std::string get_hostname_from_url(const std::string& url);

	void import_read_information(const std::string& readinfofile);
	void export_read_information(const std::string& readinfofile);

	view* v;
	urlreader* urlcfg;
	cache* rsscache;
	bool refresh_on_start;
	configcontainer cfg;
	rss_ignores ign;
	FeedContainer feedcontainer;
	filtercontainer filters;

	std::mutex reload_mutex;
	configparser cfgparser;
	colormanager colorman;
	regexmanager rxman;
	remote_api* api;
	std::mutex feeds_mutex;

	std::unique_ptr<FSLock> fslock;

	ConfigPaths configpaths;

	Reloader reloader;
};

} // namespace newsboat

#endif /* NEWSBOAT_CONTROLLER_H_ */

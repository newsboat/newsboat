#include "controller.h"

#include <cassert>
#include <cerrno>
#include <cstdlib>
#include <ctime>
#include <curl/curl.h>
#include <fstream>
#include <functional>
#include <iostream>
#include <langinfo.h>
#include <libgen.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xmlsave.h>
#include <libxml/xmlversion.h>
#include <mutex>
#include <pwd.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "cliargsparser.h"
#include "colormanager.h"
#include "config.h"
#include "configcontainer.h"
#include "configparser.h"
#include "downloadthread.h"
#include "exception.h"
#include "exceptions.h"
#include "feedhqapi.h"
#include "fileurlreader.h"
#include "globals.h"
#include "inoreaderapi.h"
#include "itemrenderer.h"
#include "logger.h"
#include "newsblurapi.h"
#include "ocnewsapi.h"
#include "oldreaderapi.h"
#include "opmlurlreader.h"
#include "regexmanager.h"
#include "remoteapi.h"
#include "rssparser.h"
#include "stflpp.h"
#include "strprintf.h"
#include "ttrssapi.h"
#include "utils.h"
#include "view.h"

namespace newsboat {

void sighup_action(int /* sig */)
{
	LOG(Level::DEBUG, "caught SIGHUP");
	Stfl::reset();
	::exit(EXIT_FAILURE);
}

void ignore_signal(int sig)
{
	LOG(Level::WARN, "caught signal %d but ignored it", sig);
}

Controller::Controller()
	: v(0)
	, urlcfg(0)
	, rsscache(0)
	, refresh_on_start(false)
	, api(0)
	, queueManager(&cfg, &configpaths)
{
}

Controller::~Controller()
{
	delete rsscache;
	delete urlcfg;
	delete api;

	feedcontainer.clear_feeds_items();
	feedcontainer.feeds.clear();
}

void Controller::set_view(View* vv)
{
	v = vv;
}

int Controller::run(const CliArgsParser& args)
{
	::signal(SIGINT, View::ctrl_c_action);
	::signal(SIGPIPE, ignore_signal);
	::signal(SIGHUP, sighup_action);

	if (!configpaths.initialized()) {
		std::cerr << configpaths.error_message() << std::endl;
		return EXIT_FAILURE;
	}

	refresh_on_start = args.refresh_on_start;

	if (args.set_log_file) {
		Logger::set_logfile(args.log_file);
	}

	if (args.set_log_level) {
		Logger::set_loglevel(args.log_level);
	}

	if (!args.display_msg.empty()) {
		std::cerr << args.display_msg << std::endl;
	}

	if (args.should_return) {
		return args.return_code;
	}

	configpaths.process_args(args);

	if (!configpaths.setup_dirs()) {
		return EXIT_FAILURE;
	}

	if (args.do_import) {
		LOG(Level::INFO,
			"Importing OPML file from %s",
			args.importfile);
		urlcfg = new FileUrlReader(configpaths.url_file());
		urlcfg->reload();
		import_opml(args.importfile);
		return EXIT_SUCCESS;
	}

	LOG(Level::INFO, "nl_langinfo(CODESET): %s", nl_langinfo(CODESET));

	if (!args.do_export) {
		if (!args.silent)
			std::cout << strprintf::fmt(_("Starting %s %s..."),
					     PROGRAM_NAME,
					     PROGRAM_VERSION)
				  << std::endl;

		fslock = std::unique_ptr<FsLock>(new FsLock());
		pid_t pid;
		if (!fslock->try_lock(configpaths.lock_file(), pid)) {
			if (!args.execute_cmds) {
				std::cout << strprintf::fmt(
						     _("Error: an instance of "
						       "%s is already running "
						       "(PID: %u)"),
						     PROGRAM_NAME,
						     pid)
					  << std::endl;
			}
			return EXIT_FAILURE;
		}
	}

	if (!args.silent)
		std::cout << _("Loading configuration...");
	std::cout.flush();

	cfg.register_commands(cfgparser);
	colorman.register_commands(cfgparser);

	KeyMap keys(KM_NEWSBOAT);
	cfgparser.register_handler("bind-key", &keys);
	cfgparser.register_handler("unbind-key", &keys);
	cfgparser.register_handler("macro", &keys);

	cfgparser.register_handler("ignore-article", &ign);
	cfgparser.register_handler("always-download", &ign);
	cfgparser.register_handler("reset-unread-on-update", &ign);

	cfgparser.register_handler("define-filter", &filters);
	cfgparser.register_handler("highlight", &rxman);
	cfgparser.register_handler("highlight-article", &rxman);

	try {
		cfgparser.parse("/etc/" PROGRAM_NAME "/config");
		cfgparser.parse(configpaths.config_file());
	} catch (const ConfigException& ex) {
		LOG(Level::ERROR,
			"an exception occurred while parsing the configuration "
			"file: %s",
			ex.what());
		std::cout << ex.what() << std::endl;
		return EXIT_FAILURE;
	}

	update_config();

	if (!args.silent)
		std::cout << _("done.") << std::endl;

	// create cache object
	std::string cachefilepath = cfg.get_configvalue("cache-file");
	if (cachefilepath.length() > 0 && !args.set_cache_file) {
		configpaths.set_cache_file(cachefilepath);
		fslock = std::unique_ptr<FsLock>(new FsLock());
		pid_t pid;
		if (!fslock->try_lock(configpaths.lock_file(), pid)) {
			std::cout << strprintf::fmt(
					     _("Error: an instance of %s is "
					       "already running (PID: %u)"),
					     PROGRAM_NAME,
					     pid)
				  << std::endl;
			return EXIT_FAILURE;
		}
	}

	if (!args.silent) {
		std::cout << _("Opening cache...");
		std::cout.flush();
	}
	try {
		rsscache = new Cache(configpaths.cache_file(), &cfg);
	} catch (const DbException& e) {
		std::cerr << strprintf::fmt(
				     _("Error: opening the cache file `%s' "
				       "failed: %s"),
				     configpaths.cache_file(),
				     e.what())
			  << std::endl;
		return EXIT_FAILURE;
	} catch (const std::runtime_error& e) {
		std::cerr << strprintf::fmt(
				     _("Error: opening the cache file `%s' "
				       "failed: %s"),
				     configpaths.cache_file(),
				     e.what())
			  << std::endl;
		return EXIT_FAILURE;
	}

	if (!args.silent) {
		std::cout << _("done.") << std::endl;
	}

	reloader =
		std::unique_ptr<Reloader>(new Reloader(this, rsscache, &cfg));

	std::string type = cfg.get_configvalue("urls-source");
	if (type == "local") {
		urlcfg = new FileUrlReader(configpaths.url_file());
	} else if (type == "opml") {
		urlcfg = new OpmlUrlReader(&cfg);
	} else if (type == "oldreader") {
		api = new OldReaderApi(&cfg);
		urlcfg = new OldReaderUrlReader(
			&cfg, configpaths.url_file(), api);
	} else if (type == "ttrss") {
		api = new TtRssApi(&cfg);
		urlcfg = new TtRssUrlReader(configpaths.url_file(), api);
	} else if (type == "newsblur") {
		const auto cookies = cfg.get_configvalue("cookie-cache");
		if (cookies.empty()) {
			std::cout << strprintf::fmt(
				_("ERROR: You must set `cookie-cache` to use "
				  "NewsBlur.\n"));
			return EXIT_FAILURE;
		}

		std::ofstream check(cookies);
		if (!check.is_open()) {
			std::cout << strprintf::fmt(
				_("%s is inaccessible and can't be created\n"),
				cookies);
			return EXIT_FAILURE;
		}

		api = new NewsBlurApi(&cfg);
		urlcfg = new NewsBlurUrlReader(configpaths.url_file(), api);
	} else if (type == "feedhq") {
		api = new FeedHqApi(&cfg);
		urlcfg = new FeedHqUrlReader(&cfg, configpaths.url_file(), api);
	} else if (type == "ocnews") {
		api = new OcNewsApi(&cfg);
		urlcfg = new OcNewsUrlReader(configpaths.url_file(), api);
	} else if (type == "inoreader") {
		api = new InoreaderApi(&cfg);
		urlcfg = new InoreaderUrlReader(
			&cfg, configpaths.url_file(), api);
	} else {
		LOG(Level::ERROR,
			"unknown urls-source `%s'",
			urlcfg->get_source());
	}

	if (!args.do_export && !args.silent) {
		std::cout << strprintf::fmt(
			_("Loading URLs from %s..."), urlcfg->get_source());
		std::cout.flush();
	}
	if (api) {
		if (!api->authenticate()) {
			std::cout << "Authentication failed." << std::endl;
			return EXIT_FAILURE;
		}
	}
	urlcfg->reload();
	if (!args.do_export && !args.silent) {
		std::cout << _("done.") << std::endl;
	}

	if (urlcfg->get_urls().size() == 0) {
		LOG(Level::ERROR, "no URLs configured.");
		std::string msg;
		if (type == "local") {
			msg = strprintf::fmt(
				_("Error: no URLs configured. Please fill the "
				  "file %s with RSS feed URLs or import an "
				  "OPML file."),
				configpaths.url_file());
		} else if (type == "opml") {
			msg = strprintf::fmt(
				_("It looks like the OPML feed you subscribed "
				  "contains no feeds. Please fill it with "
				  "feeds, and try again."));
		} else if (type == "oldreader") {
			msg = strprintf::fmt(
				_("It looks like you haven't configured any "
				  "feeds in your The Old Reader account. "
				  "Please do so, and try again."));
		} else if (type == "ttrss") {
			msg = strprintf::fmt(
				_("It looks like you haven't configured any "
				  "feeds in your Tiny Tiny RSS account. Please "
				  "do so, and try again."));
		} else if (type == "newsblur") {
			msg = strprintf::fmt(
				_("It looks like you haven't configured any "
				  "feeds in your NewsBlur account. Please do "
				  "so, and try again."));
		} else if (type == "inoreader") {
			msg = strprintf::fmt(
				_("It looks like you haven't configured any "
				  "feeds in your Inoreader account. Please do "
				  "so, and try again."));
		} else {
			assert(0); // shouldn't happen
		}
		std::cout << msg << std::endl << std::endl;
		return EXIT_FAILURE;
	}

	if (!args.do_export && !args.do_vacuum && !args.silent)
		std::cout << _("Loading articles from cache...");
	if (args.do_vacuum)
		std::cout << _("Opening cache...");
	std::cout.flush();

	if (args.do_vacuum) {
		std::cout << _("done.") << std::endl;
		std::cout << _("Cleaning up cache thoroughly...");
		std::cout.flush();
		rsscache->do_vacuum();
		std::cout << _("done.") << std::endl;
		return EXIT_SUCCESS;
	}

	unsigned int i = 0;
	for (const auto& url : urlcfg->get_urls()) {
		try {
			bool ignore_disp =
				(cfg.get_configvalue("ignore-mode") ==
					"display");
			std::shared_ptr<RssFeed> feed =
				rsscache->internalize_rssfeed(
					url, ignore_disp ? &ign : nullptr);
			feed->set_tags(urlcfg->get_tags(url));
			feed->set_order(i);
			feedcontainer.add_feed(feed);
		} catch (const DbException& e) {
			std::cout << _("Error while loading feeds from "
				       "database: ")
				  << e.what() << std::endl;
			return EXIT_FAILURE;
		} catch (const std::string& str) {
			std::cout << strprintf::fmt(
					     _("Error while loading feed '%s': "
					       "%s"),
					     url,
					     str)
				  << std::endl;
			return EXIT_FAILURE;
		}
		i++;
	}

	std::vector<std::string> tags = urlcfg->get_alltags();

	if (!args.do_export && !args.silent)
		std::cout << _("done.") << std::endl;

	// if configured, we fill all query feeds with some data; no need to
	// sort it, it will be refilled when actually opening it.
	if (cfg.get_configvalue_as_bool("prepopulate-query-feeds")) {
		if (!args.do_export && !args.silent) {
			std::cout << _("Prepopulating query feeds...");
			std::cout.flush();
		}

		feedcontainer.populate_query_feeds();

		if (!args.do_export && !args.silent) {
			std::cout << _("done.") << std::endl;
		}
	}

	feedcontainer.sort_feeds(cfg.get_feed_sort_strategy());

	if (args.do_export) {
		export_opml();
		return EXIT_SUCCESS;
	}

	if (args.do_read_import) {
		LOG(Level::INFO,
			"Importing read information file from %s",
			args.readinfofile);
		std::cout << _("Importing list of read articles...");
		std::cout.flush();
		import_read_information(args.readinfofile);
		std::cout << _("done.") << std::endl;
		return EXIT_SUCCESS;
	}

	if (args.do_read_export) {
		LOG(Level::INFO,
			"Exporting read information file to %s",
			args.readinfofile);
		std::cout << _("Exporting list of read articles...");
		std::cout.flush();
		export_read_information(args.readinfofile);
		std::cout << _("done.") << std::endl;
		return EXIT_SUCCESS;
	}

	// hand over the important objects to the View
	v->set_config_container(&cfg);
	v->set_keymap(&keys);
	v->set_tags(tags);
	v->set_cache(rsscache);
	v->set_filters(&filters);

	if (args.execute_cmds) {
		execute_commands(args.cmds_to_execute);
		return EXIT_SUCCESS;
	}

	// if the user wants to refresh on startup via configuration file, then
	// do so, but only if -r hasn't been supplied.
	if (!refresh_on_start &&
		cfg.get_configvalue_as_bool("refresh-on-startup")) {
		refresh_on_start = true;
	}

	FormAction::load_histories(
		configpaths.search_file(), configpaths.cmdline_file());

	// run the View
	int ret = v->run();

	unsigned int history_limit =
		cfg.get_configvalue_as_int("history-limit");
	LOG(Level::DEBUG, "Controller::run: history-limit = %u", history_limit);
	FormAction::save_histories(configpaths.search_file(),
		configpaths.cmdline_file(),
		history_limit);

	if (!args.silent) {
		std::cout << _("Cleaning up cache...");
		std::cout.flush();
	}
	try {
		std::lock_guard<std::mutex> feedslock(feeds_mutex);
		rsscache->cleanup_cache(feedcontainer.feeds);
		if (!args.silent) {
			std::cout << _("done.") << std::endl;
		}
	} catch (const DbException& e) {
		LOG(Level::USERERROR, "Cleaning up cache failed: %s", e.what());
		if (!args.silent) {
			std::cout << _("failed: ") << e.what() << std::endl;
			ret = EXIT_FAILURE;
		}
	}

	return ret;
}

void Controller::update_feedlist()
{
	std::lock_guard<std::mutex> feedslock(feeds_mutex);
	v->set_feedlist(feedcontainer.feeds);
}

void Controller::update_visible_feeds()
{
	std::lock_guard<std::mutex> feedslock(feeds_mutex);
	v->update_visible_feeds(feedcontainer.feeds);
}

void Controller::mark_all_read(const std::string& feedurl)
{
	try {
		rsscache->mark_all_read(feedurl);
	} catch (const DbException& e) {
		v->show_error(strprintf::fmt(
			_("Error: couldn't mark all feeds read: %s"),
			e.what()));
		return;
	}

	if (feedurl.empty()) { // Mark all feeds as read
		if (api) {
			std::lock_guard<std::mutex> feedslock(feeds_mutex);
			for (const auto& feed : feedcontainer.feeds) {
				api->mark_all_read(feed->rssurl());
			}
		}
		feedcontainer.mark_all_feeds_read();
	} else { // Mark a specific feed as read
		std::lock_guard<std::mutex> feedslock(feeds_mutex);
		const auto feed = feedcontainer.get_feed_by_url(feedurl);
		if (!feed) {
			return;
		}

		if (api) {
			api->mark_all_read(feed->rssurl());
		}

		feed->mark_all_items_read();
	}
}

void Controller::mark_article_read(const std::string& guid, bool read)
{
	if (api) {
		api->mark_article_read(guid, read);
	}
}

void Controller::mark_all_read(unsigned int pos)
{
	if (pos < feedcontainer.feeds.size()) {
		ScopeMeasure m("Controller::mark_all_read");
		std::lock_guard<std::mutex> feedslock(feeds_mutex);
		const auto feed = feedcontainer.get_feed(pos);
		if (feed->is_query_feed()) {
			rsscache->mark_all_read(feed);
		} else {
			rsscache->mark_all_read(feed->rssurl());
			if (api) {
				api->mark_all_read(feed->rssurl());
			}
		}
		m.stopover(
			"after rsscache->mark_all_read, before iteration over "
			"items");

		feedcontainer.mark_all_feed_items_read(pos);
	}
}

void Controller::replace_feed(std::shared_ptr<RssFeed> oldfeed,
	std::shared_ptr<RssFeed> newfeed,
	unsigned int pos,
	bool unattended)
{
	std::lock_guard<std::mutex> feedslock(feeds_mutex);

	LOG(Level::DEBUG, "Controller::replace_feed: feed is nonempty, saving");
	rsscache->externalize_rssfeed(
		newfeed, ign.matches_resetunread(newfeed->rssurl()));
	LOG(Level::DEBUG,
		"Controller::replace_feed: after externalize_rssfeed");

	bool ignore_disp = (cfg.get_configvalue("ignore-mode") == "display");
	std::shared_ptr<RssFeed> feed = rsscache->internalize_rssfeed(
		oldfeed->rssurl(), ignore_disp ? &ign : nullptr);
	LOG(Level::DEBUG,
		"Controller::replace_feed: after internalize_rssfeed");

	feed->set_tags(urlcfg->get_tags(oldfeed->rssurl()));
	feed->set_order(oldfeed->get_order());
	feedcontainer.feeds[pos] = feed;
	queueManager.autoenqueue(feed);
	for (const auto& item : feed->items()) {
		rsscache->update_rssitem_unread_and_enqueued(item, feed->rssurl());
	}

	oldfeed->clear_items();

	v->notify_itemlist_change(feedcontainer.feeds[pos]);
	if (!unattended) {
		v->set_feedlist(feedcontainer.feeds);
	}
}

void Controller::import_opml(const std::string& filename)
{
	if (!opml::import(filename, urlcfg)) {
		std::cout << strprintf::fmt(
				     _("An error occurred while parsing %s."),
				     filename)
			  << std::endl;
		return;
	} else {
		std::cout << strprintf::fmt(
				     _("Import of %s finished."), filename)
			  << std::endl;
	}
}

void Controller::export_opml()
{
	xmlDocPtr root = opml::generate(feedcontainer);

	xmlSaveCtxtPtr savectx = xmlSaveToFd(1, nullptr, 1);
	xmlSaveDoc(savectx, root);
	xmlSaveClose(savectx);

	xmlFreeDoc(root);
}

std::vector<std::shared_ptr<RssItem>> Controller::search_for_items(
	const std::string& query,
	std::shared_ptr<RssFeed> feed)
{
	std::vector<std::shared_ptr<RssItem>> items;
	if (feed && feed->is_query_feed()) {
		std::unordered_set<std::string> guids;
		for (const auto& item : feed->items()) {
			if (!item->deleted()) {
				guids.insert(item->guid());
			}
		}
		guids = rsscache->search_in_items(query, guids);
		for (const auto& item : feed->items()) {
			if (guids.find(item->guid()) != guids.end()) {
				items.push_back(item);
			}
		}
	} else {
		items = rsscache->search_for_items(
			query, (feed != nullptr ? feed->rssurl() : ""));
		for (const auto& item : items) {
			item->set_feedptr(
				feedcontainer.get_feed_by_url(item->feedurl()));
		}
	}
	return items;
}

void Controller::enqueue_url(std::shared_ptr<RssItem> item,
	std::shared_ptr<RssFeed> feed)
{
	queueManager.enqueue_url(item, feed);
}

void Controller::reload_urls_file()
{
	urlcfg->reload();
	std::vector<std::shared_ptr<RssFeed>> new_feeds;
	unsigned int i = 0;

	for (const auto& url : urlcfg->get_urls()) {
		const auto feed = feedcontainer.get_feed_by_url(url);
		if (feed) {
			feed->set_tags(urlcfg->get_tags(url));
			feed->set_order(i);
			new_feeds.push_back(feed);
		} else {
			try {
				bool ignore_disp =
					(cfg.get_configvalue("ignore-mode") ==
						"display");
				std::shared_ptr<RssFeed> new_feed =
					rsscache->internalize_rssfeed(url,
						ignore_disp ? &ign : nullptr);
				new_feed->set_tags(urlcfg->get_tags(url));
				new_feed->set_order(i);
				new_feeds.push_back(new_feed);
			} catch (const DbException& e) {
				LOG(Level::ERROR,
					"Controller::reload_urls_file: caught "
					"exception: %s",
					e.what());
				throw;
			}
		}
		i++;
	}

	v->set_tags(urlcfg->get_alltags());

	feedcontainer.set_feeds(new_feeds);
	feedcontainer.sort_feeds(cfg.get_feed_sort_strategy());
	update_feedlist();
}

void Controller::edit_urls_file()
{
	const char* editor;

	editor = getenv("VISUAL");
	if (!editor)
		editor = getenv("EDITOR");
	if (!editor)
		editor = "vi";

	std::string cmdline = strprintf::fmt("%s \"%s\"",
		editor,
		utils::replace_all(configpaths.url_file(), "\"", "\\\""));

	v->push_empty_formaction();
	Stfl::reset();

	utils::run_interactively(cmdline, "Controller::edit_urls_file");

	v->pop_current_formaction();

	reload_urls_file();
}

int Controller::execute_commands(const std::vector<std::string>& cmds)
{
	if (v->formaction_stack_size() > 0)
		v->pop_current_formaction();
	for (const auto& cmd : cmds) {
		LOG(Level::DEBUG,
			"Controller::execute_commands: executing `%s'",
			cmd);
		if (cmd == "reload") {
			reloader->reload_all(true);
		} else if (cmd == "print-unread") {
			std::cout << strprintf::fmt(_("%u unread articles"),
					     rsscache->get_unread_count())
				  << std::endl;
		} else {
			std::cerr
				<< strprintf::fmt(_("%s: %s: unknown command"),
					   "newsboat",
					   cmd)
				<< std::endl;
			return EXIT_FAILURE;
		}
	}
	return EXIT_SUCCESS;
}

std::string Controller::write_temporary_item(std::shared_ptr<RssItem> item)
{
	char filename[_POSIX_PATH_MAX];
	char* tmpdir = getenv("TMPDIR");
	if (tmpdir != nullptr) {
		snprintf(filename,
			sizeof(filename),
			"%s/newsboat-article.XXXXXX",
			tmpdir);
	} else {
		snprintf(filename,
			sizeof(filename),
			"/tmp/newsboat-article.XXXXXX");
	}
	int fd = mkstemp(filename);
	if (fd != -1) {
		write_item(item, filename);
		close(fd);
		return std::string(filename);
	} else {
		return "";
	}
}

void Controller::write_item(std::shared_ptr<RssItem> item,
	const std::string& filename)
{
	std::fstream f;
	f.open(filename.c_str(), std::fstream::out);
	if (!f.is_open()) {
		throw Exception(errno);
	}

	write_item(item, f);
}

void Controller::write_item(std::shared_ptr<RssItem> item, std::ostream& ostr)
{
	ostr << item_renderer::to_plain_text(cfg, item) << std::endl;
}

void Controller::import_read_information(const std::string& readinfofile)
{
	std::vector<std::string> guids;

	std::ifstream f(readinfofile.c_str());
	std::string line;
	getline(f, line);
	if (!f.is_open()) {
		return;
	}
	while (f.is_open() && !f.eof()) {
		guids.push_back(line);
		getline(f, line);
	}
	rsscache->mark_items_read_by_guid(guids);
}

void Controller::export_read_information(const std::string& readinfofile)
{
	std::vector<std::string> guids = rsscache->get_read_item_guids();

	std::fstream f;
	f.open(readinfofile.c_str(), std::fstream::out);
	if (f.is_open()) {
		for (const auto& guid : guids) {
			f << guid << std::endl;
		}
	}
}

void Controller::update_config()
{
	v->set_regexmanager(&rxman);
	v->update_bindings();

	if (colorman.colors_loaded()) {
		v->set_colors(colorman.get_fgcolors(),
			colorman.get_bgcolors(),
			colorman.get_attributes());
		v->apply_colors_to_all_formactions();
	}

	if (cfg.get_configvalue("error-log").length() > 0) {
		try {
			Logger::set_user_error_logfile(cfg.get_configvalue("error-log"));
		} catch (const Exception& e) {
			const std::string msg =
				strprintf::fmt("Couldn't open %s: %s",
					cfg.get_configvalue("error-log"),
					e.what());
			v->show_error(msg);
			std::cerr << msg << std::endl;
		}
	}
}

void Controller::load_configfile(const std::string& filename)
{
	if (cfgparser.parse(filename)) {
		update_config();
	} else {
		v->show_error(strprintf::fmt(
			_("Error: couldn't open configuration file `%s'!"),
			filename));
	}
}

void Controller::dump_config(const std::string& filename)
{
	std::vector<std::string> configlines;
	cfg.dump_config(configlines);
	if (v) {
		v->get_keys()->dump_config(configlines);
	}
	ign.dump_config(configlines);
	filters.dump_config(configlines);
	colorman.dump_config(configlines);
	rxman.dump_config(configlines);
	std::fstream f;
	f.open(filename.c_str(), std::fstream::out);
	if (f.is_open()) {
		for (const auto& line : configlines) {
			f << line << std::endl;
		}
	}
}

void Controller::update_flags(std::shared_ptr<RssItem> item)
{
	if (api) {
		api->update_article_flags(
			item->oldflags(), item->flags(), item->guid());
	}
	item->update_flags();
}

} // namespace newsboat

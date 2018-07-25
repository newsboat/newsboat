#include "controller.h"

#include <algorithm>
#include <cassert>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <curl/curl.h>
#include <fstream>
#include <functional>
#include <iostream>
#include <langinfo.h>
#include <libgen.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/uri.h>
#include <libxml/xmlsave.h>
#include <libxml/xmlversion.h>
#include <mutex>
#include <ncurses.h>
#include <pwd.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/utsname.h>
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
#include "feedhq_api.h"
#include "formatstring.h"
#include "globals.h"
#include "inoreader_api.h"
#include "logger.h"
#include "newsblur_api.h"
#include "ocnews_api.h"
#include "oldreader_api.h"
#include "regexmanager.h"
#include "remote_api.h"
#include "rss_parser.h"
#include "stflpp.h"
#include "strprintf.h"
#include "ttrss_api.h"
#include "utils.h"
#include "view.h"
#include "xlicense.h"

namespace newsboat {

int ctrl_c_hit = 0;

void ctrl_c_action(int /* sig */)
{
	LOG(level::DEBUG, "caught SIGINT");
	ctrl_c_hit = 1;
}

void sighup_action(int /* sig */)
{
	LOG(level::DEBUG, "caught SIGHUP");
	stfl::reset();
	::exit(EXIT_FAILURE);
}

void ignore_signal(int sig)
{
	LOG(level::WARN, "caught signal %d but ignored it", sig);
}

void omg_a_child_died(int /* sig */)
{
	pid_t pid;
	int stat;
	while ((pid = waitpid(-1, &stat, WNOHANG)) > 0) {
	}
	::signal(SIGCHLD, omg_a_child_died); /* in case of unreliable signals */
}

controller::controller()
	: v(0)
	, urlcfg(0)
	, rsscache(0)
	, refresh_on_start(false)
	, api(0)
	, reloader(this)
{
}

controller::~controller()
{
	delete rsscache;
	delete urlcfg;
	delete api;

	feedcontainer.clear_feeds_items();
	feedcontainer.feeds.clear();
}

void controller::set_view(view* vv)
{
	v = vv;
}

int controller::run(int argc, char* argv[])
{
	::signal(SIGINT, ctrl_c_action);
	::signal(SIGPIPE, ignore_signal);
	::signal(SIGHUP, sighup_action);
	::signal(SIGCHLD, omg_a_child_died);

	if (!configpaths.initialized()) {
		std::cerr << configpaths.error_message() << std::endl;
		return EXIT_FAILURE;
	}

	CLIArgsParser args(argc, argv);

	refresh_on_start = args.refresh_on_start;

	if (args.set_log_file) {
		logger::getInstance().set_logfile(args.log_file);
	}

	if (args.set_log_level) {
		logger::getInstance().set_loglevel(args.log_level);
	}

	if (!args.display_msg.empty()) {
		std::cerr << args.display_msg << std::endl;
	}

	if (args.should_print_usage) {
		print_usage(argv[0]);
	}

	if (args.should_return) {
		return args.return_code;
	}

	if (args.show_version) {
		print_version_information(argv[0], args.show_version);
		return EXIT_SUCCESS;
	}

	configpaths.process_args(args);

	if (args.do_import) {
		LOG(level::INFO,
			"Importing OPML file from %s",
			args.importfile);
		urlcfg = new file_urlreader(configpaths.url_file());
		urlcfg->reload();
		import_opml(args.importfile);
		return EXIT_SUCCESS;
	}

	LOG(level::INFO, "nl_langinfo(CODESET): %s", nl_langinfo(CODESET));

	if (!configpaths.setup_dirs()) {
		return EXIT_FAILURE;
	}

	if (!args.do_export) {
		if (!args.silent)
			std::cout << strprintf::fmt(_("Starting %s %s..."),
					     PROGRAM_NAME,
					     PROGRAM_VERSION)
				  << std::endl;

		fslock = std::unique_ptr<FSLock>(new FSLock());
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

	keymap keys(KM_NEWSBOAT);
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
	} catch (const configexception& ex) {
		LOG(level::ERROR,
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
		fslock = std::unique_ptr<FSLock>(new FSLock());
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
		rsscache = new cache(configpaths.cache_file(), &cfg);
	} catch (const dbexception& e) {
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

	std::string type = cfg.get_configvalue("urls-source");
	if (type == "local") {
		urlcfg = new file_urlreader(configpaths.url_file());
	} else if (type == "opml") {
		urlcfg = new opml_urlreader(&cfg);
	} else if (type == "oldreader") {
		api = new oldreader_api(&cfg);
		urlcfg = new oldreader_urlreader(
			&cfg, configpaths.url_file(), api);
	} else if (type == "ttrss") {
		api = new ttrss_api(&cfg);
		urlcfg = new ttrss_urlreader(configpaths.url_file(), api);
	} else if (type == "newsblur") {
		auto cookies = cfg.get_configvalue("cookie-cache");
		std::ofstream check(cookies);
		if (!check.is_open()) {
			std::cout << strprintf::fmt(
				_("%s is inaccessible and can't be created\n"),
				cookies);
			return EXIT_FAILURE;
		}

		api = new newsblur_api(&cfg);
		urlcfg = new newsblur_urlreader(configpaths.url_file(), api);
	} else if (type == "feedhq") {
		api = new feedhq_api(&cfg);
		urlcfg =
			new feedhq_urlreader(&cfg, configpaths.url_file(), api);
	} else if (type == "ocnews") {
		api = new ocnews_api(&cfg);
		urlcfg = new ocnews_urlreader(configpaths.url_file(), api);
	} else if (type == "inoreader") {
		api = new inoreader_api(&cfg);
		urlcfg = new inoreader_urlreader(
			&cfg, configpaths.url_file(), api);
	} else {
		LOG(level::ERROR,
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
		LOG(level::ERROR, "no URLs configured.");
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
		print_usage(argv[0]);
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
			std::shared_ptr<rss_feed> feed =
				rsscache->internalize_rssfeed(
					url, ignore_disp ? &ign : nullptr);
			feed->set_tags(urlcfg->get_tags(url));
			feed->set_order(i);
			feedcontainer.add_feed(feed);
		} catch (const dbexception& e) {
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
		LOG(level::INFO,
			"Importing read information file from %s",
			args.readinfofile);
		std::cout << _("Importing list of read articles...");
		std::cout.flush();
		import_read_information(args.readinfofile);
		std::cout << _("done.") << std::endl;
		return EXIT_SUCCESS;
	}

	if (args.do_read_export) {
		LOG(level::INFO,
			"Exporting read information file to %s",
			args.readinfofile);
		std::cout << _("Exporting list of read articles...");
		std::cout.flush();
		export_read_information(args.readinfofile);
		std::cout << _("done.") << std::endl;
		return EXIT_SUCCESS;
	}

	// hand over the important objects to the view
	v->set_config_container(&cfg);
	v->set_keymap(&keys);
	v->set_tags(tags);

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

	formaction::load_histories(
		configpaths.search_file(), configpaths.cmdline_file());

	// run the view
	int ret = v->run();

	unsigned int history_limit =
		cfg.get_configvalue_as_int("history-limit");
	LOG(level::DEBUG, "controller::run: history-limit = %u", history_limit);
	formaction::save_histories(configpaths.search_file(),
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
	} catch (const dbexception& e) {
		LOG(level::USERERROR, "Cleaning up cache failed: %s", e.what());
		if (!args.silent) {
			std::cout << _("failed: ") << e.what() << std::endl;
			ret = EXIT_FAILURE;
		}
	}

	return ret;
}

void controller::update_feedlist()
{
	std::lock_guard<std::mutex> feedslock(feeds_mutex);
	v->set_feedlist(feedcontainer.feeds);
}

void controller::update_visible_feeds()
{
	std::lock_guard<std::mutex> feedslock(feeds_mutex);
	v->update_visible_feeds(feedcontainer.feeds);
}

void controller::mark_all_read(const std::string& feedurl)
{
	try {
		rsscache->mark_all_read(feedurl);
	} catch (const dbexception& e) {
		v->show_error(strprintf::fmt(
			_("Error: couldn't mark all feeds read: %s"),
			e.what()));
		return;
	}

	std::lock_guard<std::mutex> feedslock(feeds_mutex);
	for (const auto& feed : feedcontainer.feeds) {
		std::lock_guard<std::mutex> lock(feed->item_mutex);

		if (feedurl.length() > 0 && feed->rssurl() != feedurl)
			continue;

		if (feed->total_item_count() > 0) {
			if (api) {
				api->mark_all_read(feed->rssurl());
			}
			for (const auto& item : feed->items()) {
				item->set_unread_nowrite(false);
			}
		}

		// no point in going on - there is only one feed with a given
		// URL
		if (feedurl.length() > 0)
			break;
	}
}

void controller::mark_article_read(const std::string& guid, bool read)
{
	if (api) {
		api->mark_article_read(guid, read);
	}
}

void controller::mark_all_read(unsigned int pos)
{
	if (pos < feedcontainer.feeds.size()) {
		scope_measure m("controller::mark_all_read");
		std::lock_guard<std::mutex> feedslock(feeds_mutex);
		const auto feed = feedcontainer.get_feed(pos);
		if (feed->rssurl().substr(0, 6) == "query:") {
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

void controller::replace_feed(std::shared_ptr<rss_feed> oldfeed,
	std::shared_ptr<rss_feed> newfeed,
	unsigned int pos,
	bool unattended)
{
	std::lock_guard<std::mutex> feedslock(feeds_mutex);

	LOG(level::DEBUG, "controller::replace_feed: feed is nonempty, saving");
	rsscache->externalize_rssfeed(
		newfeed, ign.matches_resetunread(newfeed->rssurl()));
	LOG(level::DEBUG,
		"controller::replace_feed: after externalize_rssfeed");

	bool ignore_disp = (cfg.get_configvalue("ignore-mode") == "display");
	std::shared_ptr<rss_feed> feed = rsscache->internalize_rssfeed(
		oldfeed->rssurl(), ignore_disp ? &ign : nullptr);
	LOG(level::DEBUG,
		"controller::replace_feed: after internalize_rssfeed");

	feed->set_tags(urlcfg->get_tags(oldfeed->rssurl()));
	feed->set_order(oldfeed->get_order());
	feedcontainer.feeds[pos] = feed;
	enqueue_items(feed);

	oldfeed->clear_items();

	v->notify_itemlist_change(feedcontainer.feeds[pos]);
	if (!unattended) {
		v->set_feedlist(feedcontainer.feeds);
	}
}

void controller::reload(unsigned int pos,
	unsigned int max,
	bool unattended,
	curl_handle* easyhandle)
{
	LOG(level::DEBUG, "controller::reload: pos = %u max = %u", pos, max);
	if (pos < feedcontainer.feeds.size()) {
		std::shared_ptr<rss_feed> oldfeed = feedcontainer.feeds[pos];
		std::string errmsg;
		if (!unattended)
			v->set_status(strprintf::fmt(_("%sLoading %s..."),
				prepare_message(pos + 1, max),
				utils::censor_url(oldfeed->rssurl())));

		bool ignore_dl =
			(cfg.get_configvalue("ignore-mode") == "download");

		rss_parser parser(oldfeed->rssurl(),
			rsscache,
			&cfg,
			ignore_dl ? &ign : nullptr,
			api);
		parser.set_easyhandle(easyhandle);
		LOG(level::DEBUG, "controller::reload: created parser");
		try {
			oldfeed->set_status(dl_status::DURING_DOWNLOAD);
			std::shared_ptr<rss_feed> newfeed = parser.parse();
			if (newfeed->total_item_count() > 0) {
				replace_feed(oldfeed, newfeed, pos, unattended);
			} else {
				LOG(level::DEBUG,
					"controller::reload: feed is empty");
			}
			oldfeed->set_status(dl_status::SUCCESS);
			v->set_status("");
		} catch (const dbexception& e) {
			errmsg = strprintf::fmt(
				_("Error while retrieving %s: %s"),
				utils::censor_url(oldfeed->rssurl()),
				e.what());
		} catch (const std::string& emsg) {
			errmsg = strprintf::fmt(
				_("Error while retrieving %s: %s"),
				utils::censor_url(oldfeed->rssurl()),
				emsg);
		} catch (rsspp::exception& e) {
			errmsg = strprintf::fmt(
				_("Error while retrieving %s: %s"),
				utils::censor_url(oldfeed->rssurl()),
				e.what());
		}
		if (errmsg != "") {
			oldfeed->set_status(dl_status::DL_ERROR);
			v->set_status(errmsg);
			LOG(level::USERERROR, "%s", errmsg);
		}
	} else {
		v->show_error(_("Error: invalid feed!"));
	}
}

void controller::reload_indexes(const std::vector<int>& indexes,
	bool unattended)
{
	scope_measure m1("controller::reload_indexes");
	const auto unread_feeds = feedcontainer.unread_feed_count();
	const auto unread_articles = feedcontainer.unread_item_count();
	const auto size = feedcontainer.feeds_size();

	for (const auto& idx : indexes) {
		this->reload(idx, size, unattended);
	}

	const auto unread_feeds2 = feedcontainer.unread_feed_count();
	const auto unread_articles2 = feedcontainer.unread_item_count();
	bool notify_always = cfg.get_configvalue_as_bool("notify-always");
	if (notify_always || unread_feeds2 != unread_feeds ||
		unread_articles2 != unread_articles) {
		fmtstr_formatter fmt;
		fmt.register_fmt('f', std::to_string(unread_feeds2));
		fmt.register_fmt('n', std::to_string(unread_articles2));
		fmt.register_fmt('d',
			std::to_string(unread_articles2 - unread_articles));
		fmt.register_fmt(
			'D', std::to_string(unread_feeds2 - unread_feeds));
		this->notify(
			fmt.do_format(cfg.get_configvalue("notify-format")));
	}
	if (!unattended)
		v->set_status("");
}

void controller::reload_range(unsigned int start,
	unsigned int end,
	unsigned int size,
	bool unattended)
{
	std::vector<unsigned int> v;
	for (unsigned int i = start; i <= end; ++i)
		v.push_back(i);

	auto extract = [](std::string& s, const std::string& url) {
		size_t p = url.find("//");
		p = (p == std::string::npos) ? 0 : p + 2;
		std::string suff(url.substr(p));
		p = suff.find('/');
		s = suff.substr(0, p);
	};

	std::sort(v.begin(), v.end(), [&](unsigned int a, unsigned int b) {
		std::string domain1, domain2;
		extract(domain1, feedcontainer.feeds[a]->rssurl());
		extract(domain2, feedcontainer.feeds[b]->rssurl());
		std::reverse(domain1.begin(), domain1.end());
		std::reverse(domain2.begin(), domain2.end());
		return domain1 < domain2;
	});

	curl_handle easyhandle;

	for (const auto& i : v) {
		LOG(level::DEBUG,
			"controller::reload_range: reloading feed #%u",
			i);
		this->reload(i, size, unattended, &easyhandle);
	}
}

void controller::reload_all(bool unattended)
{
	const auto unread_feeds = feedcontainer.unread_feed_count();
	const auto unread_articles = feedcontainer.unread_item_count();
	int num_threads = cfg.get_configvalue_as_int("reload-threads");
	time_t t1, t2, dt;

	feedcontainer.reset_feeds_status();
	const auto num_feeds = feedcontainer.feeds_size();

	// TODO: change to std::clamp in C++17
	const int min_threads = 1;
	const int max_threads = num_feeds;
	num_threads = std::max(min_threads, std::min(num_threads, max_threads));

	t1 = time(nullptr);

	LOG(level::DEBUG,
		"controller::reload_all: starting with reload all...");
	if (num_threads == 1) {
		this->reload_range(0, num_feeds - 1, num_feeds, unattended);
	} else {
		std::vector<std::pair<unsigned int, unsigned int>> partitions =
			utils::partition_indexes(0, num_feeds - 1, num_threads);
		std::vector<std::thread> threads;
		LOG(level::DEBUG,
			"controller::reload_all: starting reload threads...");
		for (int i = 0; i < num_threads - 1; i++) {
			threads.push_back(std::thread(reloadrangethread(this,
				partitions[i].first,
				partitions[i].second,
				num_feeds,
				unattended)));
		}
		LOG(level::DEBUG,
			"controller::reload_all: starting my own reload...");
		this->reload_range(partitions[num_threads - 1].first,
			partitions[num_threads - 1].second,
			num_feeds,
			unattended);
		LOG(level::DEBUG,
			"controller::reload_all: joining other threads...");
		for (size_t i = 0; i < threads.size(); i++) {
			threads[i].join();
		}
	}

	// refresh query feeds (update and sort)
	LOG(level::DEBUG, "controller::reload_all: refresh query feeds");
	for (const auto& feed : feedcontainer.feeds) {
		v->prepare_query_feed(feed);
	}
	v->force_redraw();

	feedcontainer.sort_feeds(cfg.get_feed_sort_strategy());
	update_feedlist();

	t2 = time(nullptr);
	dt = t2 - t1;
	LOG(level::INFO, "controller::reload_all: reload took %d seconds", dt);

	const auto unread_feeds2 = feedcontainer.unread_feed_count();
	const auto unread_articles2 = feedcontainer.unread_item_count();
	bool notify_always = cfg.get_configvalue_as_bool("notify-always");
	if (notify_always || unread_feeds2 > unread_feeds ||
		unread_articles2 > unread_articles) {
		int article_count = unread_articles2 - unread_articles;
		int feed_count = unread_feeds2 - unread_feeds;

		LOG(level::DEBUG, "unread article count: %d", article_count);
		LOG(level::DEBUG, "unread feed count: %d", feed_count);

		fmtstr_formatter fmt;
		fmt.register_fmt('f', std::to_string(unread_feeds2));
		fmt.register_fmt('n', std::to_string(unread_articles2));
		fmt.register_fmt('d',
			std::to_string(article_count >= 0 ? article_count : 0));
		fmt.register_fmt(
			'D', std::to_string(feed_count >= 0 ? feed_count : 0));
		this->notify(
			fmt.do_format(cfg.get_configvalue("notify-format")));
	}
}

void controller::notify(const std::string& msg)
{
	if (cfg.get_configvalue_as_bool("notify-screen")) {
		LOG(level::DEBUG, "controller:notify: notifying screen");
		std::cout << "\033^" << msg << "\033\\";
		std::cout.flush();
	}
	if (cfg.get_configvalue_as_bool("notify-xterm")) {
		LOG(level::DEBUG, "controller:notify: notifying xterm");
		std::cout << "\033]2;" << msg << "\033\\";
		std::cout.flush();
	}
	if (cfg.get_configvalue_as_bool("notify-beep")) {
		LOG(level::DEBUG, "controller:notify: notifying beep");
		::beep();
	}
	if (cfg.get_configvalue("notify-program").length() > 0) {
		std::string prog = cfg.get_configvalue("notify-program");
		LOG(level::DEBUG,
			"controller:notify: notifying external program `%s'",
			prog);
		utils::run_command(prog, msg);
	}
}

void controller::print_version_information(const char* argv0,
	unsigned int level)
{
	if (level <= 1) {
		std::cout << PROGRAM_NAME << " " << PROGRAM_VERSION << " - "
			  << PROGRAM_URL << std::endl;
		std::cout << "Copyright (C) 2006-2015 Andreas Krennmair"
			  << std::endl;
		std::cout << "Copyright (C) 2015-2018 Alexander Batischev"
			  << std::endl;
		std::cout << "Copyright (C) 2006-2017 Newsbeuter contributors"
			  << std::endl;
		std::cout << "Copyright (C) 2017-2018 Newsboat contributors"
			  << std::endl;
		std::cout << std::endl;

		std::cout << strprintf::fmt(
				     _("Newsboat is free software licensed "
				       "under the MIT License. (Type `%s -vv' "
				       "to see the full text.)"),
				     argv0)
			  << std::endl;
		std::cout << _("It bundles JSON for Modern C++ library, "
			       "licensed under the MIT License: "
			       "https://github.com/nlohmann/json")
			  << std::endl;
		std::cout << std::endl;

		struct utsname xuts;
		uname(&xuts);
		std::cout << PROGRAM_NAME << " " << PROGRAM_VERSION
			  << std::endl;
		std::cout << "System: " << xuts.sysname << " " << xuts.release
			  << " (" << xuts.machine << ")" << std::endl;
#if defined(__GNUC__) && defined(__VERSION__)
		std::cout << "Compiler: g++ " << __VERSION__ << std::endl;
#endif
		std::cout << "ncurses: " << curses_version()
			  << " (compiled with " << NCURSES_VERSION << ")"
			  << std::endl;
		std::cout << "libcurl: " << curl_version() << " (compiled with "
			  << LIBCURL_VERSION << ")" << std::endl;
		std::cout << "SQLite: " << sqlite3_libversion()
			  << " (compiled with " << SQLITE_VERSION << ")"
			  << std::endl;
		std::cout << "libxml2: compiled with " << LIBXML_DOTTED_VERSION
			  << std::endl
			  << std::endl;
	} else {
		std::cout << LICENSE_str << std::endl;
	}
}

void controller::print_usage(char* argv0)
{
	auto msg = strprintf::fmt(
		_("%s %s\nusage: %s [-i <file>|-e] [-u <urlfile>] "
		  "[-c <cachefile>] [-x <command> ...] [-h]\n"),
		PROGRAM_NAME,
		PROGRAM_VERSION,
		argv0);
	std::cout << msg;

	struct arg {
		const char name;
		const std::string longname;
		const std::string params;
		const std::string desc;
	};

	static const std::vector<arg> args = {
		{'e', "export-to-opml", "", _s("export OPML feed to stdout")},
		{'r', "refresh-on-start", "", _s("refresh feeds on start")},
		{'i', "import-from-opml", _s("<file>"), _s("import OPML file")},
		{'u',
			"url-file",
			_s("<urlfile>"),
			_s("read RSS feed URLs from <urlfile>")},
		{'c',
			"cache-file",
			_s("<cachefile>"),
			_s("use <cachefile> as cache file")},
		{'C',
			"config-file",
			_s("<configfile>"),
			_s("read configuration from <configfile>")},
		{'X', "vacuum", "", _s("compact the cache")},
		{'x',
			"execute",
			_s("<command>..."),
			_s("execute list of commands")},
		{'q', "quiet", "", _s("quiet startup")},
		{'v', "version", "", _s("get version information")},
		{'l',
			"log-level",
			_s("<loglevel>"),
			_s("write a log with a certain loglevel (valid values: "
			   "1 to "
			   "6)")},
		{'d',
			"log-file",
			_s("<logfile>"),
			_s("use <logfile> as output log file")},
		{'E',
			"export-to-file",
			_s("<file>"),
			_s("export list of read articles to <file>")},
		{'I',
			"import-from-file",
			_s("<file>"),
			_s("import list of read articles from <file>")},
		{'h', "help", "", _s("this help")}};

	for (const auto& a : args) {
		std::string longcolumn("-");
		longcolumn += a.name;
		longcolumn += ", --" + a.longname;
		longcolumn += a.params.size() > 0 ? "=" + a.params : "";
		std::cout << "\t" << longcolumn;
		for (unsigned int j = 0; j < utils::gentabs(longcolumn); j++) {
			std::cout << "\t";
		}
		std::cout << a.desc << std::endl;
	}
}

void controller::import_opml(const std::string& filename)
{
	xmlDoc* doc = xmlReadFile(filename.c_str(), nullptr, 0);
	if (doc == nullptr) {
		std::cout << strprintf::fmt(
				     _("An error occurred while parsing %s."),
				     filename)
			  << std::endl;
		return;
	}

	xmlNode* root = xmlDocGetRootElement(doc);

	for (xmlNode* node = root->children; node != nullptr;
		node = node->next) {
		if (strcmp((const char*)node->name, "body") == 0) {
			LOG(level::DEBUG, "import_opml: found body");
			rec_find_rss_outlines(node->children, "");
			urlcfg->write_config();
		}
	}

	xmlFreeDoc(doc);
	std::cout << strprintf::fmt(_("Import of %s finished."), filename)
		  << std::endl;
}

void controller::export_opml()
{
	xmlDocPtr root = xmlNewDoc((const xmlChar*)"1.0");
	xmlNodePtr opml_node =
		xmlNewDocNode(root, nullptr, (const xmlChar*)"opml", nullptr);
	xmlSetProp(opml_node, (const xmlChar*)"version", (const xmlChar*)"1.0");
	xmlDocSetRootElement(root, opml_node);

	xmlNodePtr head = xmlNewTextChild(
		opml_node, nullptr, (const xmlChar*)"head", nullptr);
	xmlNewTextChild(head,
		nullptr,
		(const xmlChar*)"title",
		(const xmlChar*)PROGRAM_NAME " - Exported Feeds");
	xmlNodePtr body = xmlNewTextChild(
		opml_node, nullptr, (const xmlChar*)"body", nullptr);

	for (const auto& feed : feedcontainer.feeds) {
		if (!utils::is_special_url(feed->rssurl())) {
			std::string rssurl = feed->rssurl();
			std::string link = feed->link();
			std::string title = feed->title();

			xmlNodePtr outline = xmlNewTextChild(body,
				nullptr,
				(const xmlChar*)"outline",
				nullptr);
			xmlSetProp(outline,
				(const xmlChar*)"type",
				(const xmlChar*)"rss");
			xmlSetProp(outline,
				(const xmlChar*)"xmlUrl",
				(const xmlChar*)rssurl.c_str());
			xmlSetProp(outline,
				(const xmlChar*)"htmlUrl",
				(const xmlChar*)link.c_str());
			xmlSetProp(outline,
				(const xmlChar*)"title",
				(const xmlChar*)title.c_str());
		}
	}

	xmlSaveCtxtPtr savectx = xmlSaveToFd(1, nullptr, 1);
	xmlSaveDoc(savectx, root);
	xmlSaveClose(savectx);

	xmlFreeNode(opml_node);
}

void controller::rec_find_rss_outlines(xmlNode* node, std::string tag)
{
	while (node) {
		std::string newtag = tag;

		if (strcmp((const char*)node->name, "outline") == 0) {
			char* url = (char*)xmlGetProp(
				node, (const xmlChar*)"xmlUrl");
			if (!url) {
				url = (char*)xmlGetProp(
					node, (const xmlChar*)"url");
			}

			if (url) {
				LOG(level::DEBUG,
					"OPML import: found RSS outline with "
					"url = "
					"%s",
					url);

				std::string nurl = std::string(url);

				// Liferea uses a pipe to signal feeds read from
				// the output of a program in its OPMLs. Convert
				// them to our syntax.
				if (*url == '|') {
					nurl = strprintf::fmt(
						"exec:%s", url + 1);
					LOG(level::DEBUG,
						"OPML import: liferea-style "
						"url %s "
						"converted to %s",
						url,
						nurl);
				}

				// Handle OPML filters.
				char* filtercmd = (char*)xmlGetProp(
					node, (const xmlChar*)"filtercmd");
				if (filtercmd) {
					LOG(level::DEBUG,
						"OPML import: adding filter "
						"command %s to url %s",
						filtercmd,
						nurl);
					nurl.insert(0,
						strprintf::fmt("filter:%s:",
							filtercmd));
					xmlFree(filtercmd);
				}

				xmlFree(url);
				// Filters and scripts may have arguments, so,
				// quote them when needed.
				url = (char*)xmlStrdup(
					(const xmlChar*)
						utils::quote_if_necessary(nurl)
							.c_str());
				assert(url);

				bool found = false;

				LOG(level::DEBUG,
					"OPML import: size = %u",
					urlcfg->get_urls().size());
				if (urlcfg->get_urls().size() > 0) {
					for (const auto& u :
						urlcfg->get_urls()) {
						if (u == url) {
							found = true;
						}
					}
				}

				if (!found) {
					LOG(level::DEBUG,
						"OPML import: added url = %s",
						url);
					urlcfg->get_urls().push_back(
						std::string(url));
					if (tag.length() > 0) {
						LOG(level::DEBUG,
							"OPML import: "
							"appending "
							"tag %s to url %s",
							tag,
							url);
						urlcfg->get_tags(url).push_back(
							tag);
					}
				} else {
					LOG(level::DEBUG,
						"OPML import: url = %s is "
						"already "
						"in list",
						url);
				}
				xmlFree(url);
			} else {
				char* text = (char*)xmlGetProp(
					node, (const xmlChar*)"text");
				if (!text)
					text = (char*)xmlGetProp(
						node, (const xmlChar*)"title");
				if (text) {
					if (newtag.length() > 0) {
						newtag.append("/");
					}
					newtag.append(text);
					xmlFree(text);
				}
			}
		}
		rec_find_rss_outlines(node->children, newtag);

		node = node->next;
	}
}

std::vector<std::shared_ptr<rss_item>> controller::search_for_items(
	const std::string& query,
	std::shared_ptr<rss_feed> feed)
{
	std::vector<std::shared_ptr<rss_item>> items;
	if (feed != nullptr && feed->rssurl().substr(0, 6) == "query:") {
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

void controller::enqueue_url(const std::string& url,
	std::shared_ptr<rss_feed> feed)
{
	bool url_found = false;
	std::fstream f;
	f.open(configpaths.queue_file().c_str(), std::fstream::in);
	if (f.is_open()) {
		do {
			std::string line;
			getline(f, line);
			if (!f.eof() && line.length() > 0) {
				std::vector<std::string> fields =
					utils::tokenize_quoted(line);
				if (!fields.empty() && fields[0] == url) {
					url_found = true;
					break;
				}
			}
		} while (!f.eof());
		f.close();
	}
	if (!url_found) {
		f.open(configpaths.queue_file().c_str(),
			std::fstream::app | std::fstream::out);
		std::string filename = generate_enqueue_filename(url, feed);
		f << url << " " << stfl::quote(filename) << std::endl;
		f.close();
	}
}

void controller::reload_urls_file()
{
	urlcfg->reload();
	std::vector<std::shared_ptr<rss_feed>> new_feeds;
	unsigned int i = 0;

	for (const auto& url : urlcfg->get_urls()) {
		bool found = false;
		for (const auto& feed : feedcontainer.feeds) {
			if (url == feed->rssurl()) {
				found = true;
				feed->set_tags(urlcfg->get_tags(url));
				feed->set_order(i);
				new_feeds.push_back(feed);
				break;
			}
		}
		if (!found) {
			try {
				bool ignore_disp =
					(cfg.get_configvalue("ignore-mode") ==
						"display");
				std::shared_ptr<rss_feed> new_feed =
					rsscache->internalize_rssfeed(url,
						ignore_disp ? &ign : nullptr);
				new_feed->set_tags(urlcfg->get_tags(url));
				new_feed->set_order(i);
				new_feeds.push_back(new_feed);
			} catch (const dbexception& e) {
				LOG(level::ERROR,
					"controller::reload_urls_file: caught "
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

void controller::edit_urls_file()
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
	stfl::reset();

	utils::run_interactively(cmdline, "controller::edit_urls_file");

	v->pop_current_formaction();

	reload_urls_file();
}

/* When passing an argument to a shell script, empty string should be
 * represented as '' (two quote marks), otherwise shell won't be able to tell
 * that the parameter is empty */
std::string quote_empty(const std::string& input)
{
	if (input.empty()) {
		return "''";
	} else {
		return input;
	}
}

std::string controller::bookmark(const std::string& url,
	const std::string& title,
	const std::string& description,
	const std::string& feed_title)
{
	std::string bookmark_cmd = cfg.get_configvalue("bookmark-cmd");
	bool is_interactive =
		cfg.get_configvalue_as_bool("bookmark-interactive");
	if (bookmark_cmd.length() > 0) {
		std::string cmdline = strprintf::fmt("%s '%s' '%s' '%s' '%s'",
			bookmark_cmd,
			utils::replace_all(url, "'", "%27"),
			utils::replace_all(title, "'", "%27"),
			utils::replace_all(description, "'", "%27"),
			utils::replace_all(feed_title, "'", "%27"));

		LOG(level::DEBUG, "controller::bookmark: cmd = %s", cmdline);

		if (is_interactive) {
			v->push_empty_formaction();
			stfl::reset();
			utils::run_interactively(
				cmdline, "controller::bookmark");
			v->pop_current_formaction();
			return "";
		} else {
			char* my_argv[4];
			my_argv[0] = const_cast<char*>("/bin/sh");
			my_argv[1] = const_cast<char*>("-c");
			my_argv[2] = const_cast<char*>(cmdline.c_str());
			my_argv[3] = nullptr;
			return utils::run_program(my_argv, "");
		}
	} else {
		return _(
			"bookmarking support is not configured. Please set the "
			"configuration variable `bookmark-cmd' accordingly.");
	}
}

int controller::execute_commands(const std::vector<std::string>& cmds)
{
	if (v->formaction_stack_size() > 0)
		v->pop_current_formaction();
	for (const auto& cmd : cmds) {
		LOG(level::DEBUG,
			"controller::execute_commands: executing `%s'",
			cmd);
		if (cmd == "reload") {
			reload_all(true);
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

std::string controller::write_temporary_item(std::shared_ptr<rss_item> item)
{
	char filename[_POSIX_PATH_MAX];
	snprintf(filename, sizeof(filename), "/tmp/newsboat-article.XXXXXX");
	int fd = mkstemp(filename);
	if (fd != -1) {
		write_item(item, filename);
		close(fd);
		return std::string(filename);
	} else {
		return "";
	}
}

void controller::write_item(std::shared_ptr<rss_item> item,
	const std::string& filename)
{
	std::fstream f;
	f.open(filename.c_str(), std::fstream::out);
	if (!f.is_open())
		throw exception(errno);

	write_item(item, f);
}

void controller::write_item(std::shared_ptr<rss_item> item, std::ostream& ostr)
{
	std::vector<std::pair<LineType, std::string>> lines;
	std::vector<linkpair> links; // not used

	std::string title(_("Title: "));
	title.append(item->title());
	lines.push_back(std::make_pair(LineType::wrappable, title));

	std::string author(_("Author: "));
	author.append(item->author());
	lines.push_back(std::make_pair(LineType::wrappable, author));

	std::string date(_("Date: "));
	date.append(item->pubDate());
	lines.push_back(std::make_pair(LineType::wrappable, date));

	std::string link(_("Link: "));
	link.append(item->link());
	lines.push_back(std::make_pair(LineType::softwrappable, link));

	if (item->enclosure_url() != "") {
		std::string dlurl(_("Podcast Download URL: "));
		dlurl.append(item->enclosure_url());
		lines.push_back(std::make_pair(LineType::softwrappable, dlurl));
	}

	lines.push_back(std::make_pair(LineType::wrappable, std::string("")));

	htmlrenderer rnd(true);
	rnd.render(item->description(), lines, links, item->feedurl());
	textformatter txtfmt;
	txtfmt.add_lines(lines);

	unsigned int width = cfg.get_configvalue_as_int("text-width");
	if (width == 0)
		width = 80;
	ostr << txtfmt.format_text_plain(width) << std::endl;
}

void controller::mark_deleted(const std::string& guid, bool b)
{
	rsscache->mark_item_deleted(guid, b);
}

std::string controller::prepare_message(unsigned int pos, unsigned int max)
{
	if (max > 0) {
		return strprintf::fmt("(%u/%u) ", pos, max);
	}
	return "";
}

void controller::enqueue_items(std::shared_ptr<rss_feed> feed)
{
	if (!cfg.get_configvalue_as_bool("podcast-auto-enqueue"))
		return;
	std::lock_guard<std::mutex> lock(feed->item_mutex);
	for (const auto& item : feed->items()) {
		if (!item->enqueued() && item->enclosure_url().length() > 0) {
			LOG(level::DEBUG,
				"controller::enqueue_items: enclosure_url = "
				"`%s' "
				"enclosure_type = `%s'",
				item->enclosure_url(),
				item->enclosure_type());
			if (utils::is_http_url(item->enclosure_url())) {
				LOG(level::INFO,
					"controller::enqueue_items: enqueuing "
					"`%s'",
					item->enclosure_url());
				enqueue_url(item->enclosure_url(), feed);
				item->set_enqueued(true);
				rsscache->update_rssitem_unread_and_enqueued(
					item, feed->rssurl());
			}
		}
	}
}

std::string controller::generate_enqueue_filename(const std::string& url,
	std::shared_ptr<rss_feed> feed)
{
	std::string dlformat = cfg.get_configvalue("download-path");
	if (dlformat[dlformat.length() - 1] != NEWSBEUTER_PATH_SEP[0])
		dlformat.append(NEWSBEUTER_PATH_SEP);

	fmtstr_formatter fmt;
	fmt.register_fmt('n', feed->title());
	fmt.register_fmt('h', get_hostname_from_url(url));

	std::string dlpath = fmt.do_format(dlformat);

	char buf[2048];
	snprintf(buf, sizeof(buf), "%s", url.c_str());
	char* base = basename(buf);
	if (!base || strlen(base) == 0) {
		char lbuf[128];
		time_t t = time(nullptr);
		strftime(lbuf,
			sizeof(lbuf),
			"%Y-%b-%d-%H%M%S.unknown",
			localtime(&t));
		dlpath.append(lbuf);
	} else {
		dlpath.append(base);
	}
	return dlpath;
}

std::string controller::get_hostname_from_url(const std::string& url)
{
	xmlURIPtr uri = xmlParseURI(url.c_str());
	std::string hostname;
	if (uri) {
		hostname = uri->server;
		xmlFreeURI(uri);
	}
	return hostname;
}

void controller::import_read_information(const std::string& readinfofile)
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

void controller::export_read_information(const std::string& readinfofile)
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

void controller::update_config()
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
			logger::getInstance().set_errorlogfile(
				cfg.get_configvalue("error-log"));
		} catch (const exception& e) {
			const std::string msg =
				strprintf::fmt("Couldn't open %s: %s",
					cfg.get_configvalue("error-log"),
					e.what());
			v->show_error(msg);
			std::cerr << msg << std::endl;
		}
	}
}

void controller::load_configfile(const std::string& filename)
{
	if (cfgparser.parse(filename, true)) {
		update_config();
	} else {
		v->show_error(strprintf::fmt(
			_("Error: couldn't open configuration file `%s'!"),
			filename));
	}
}

void controller::dump_config(const std::string& filename)
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

void controller::update_flags(std::shared_ptr<rss_item> item)
{
	if (api) {
		api->update_article_flags(
			item->oldflags(), item->flags(), item->guid());
	}
	item->update_flags();
}

} // namespace newsboat

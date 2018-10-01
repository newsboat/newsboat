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
#include <libxml/uri.h>
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
#include "formatstring.h"
#include "globals.h"
#include "inoreaderapi.h"
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

void omg_a_child_died(int /* sig */)
{
	pid_t pid;
	int stat;
	while ((pid = waitpid(-1, &stat, WNOHANG)) > 0) {
	}
	::signal(SIGCHLD, omg_a_child_died); /* in case of unreliable signals */
}

Controller::Controller()
	: v(0)
	, urlcfg(0)
	, rsscache(0)
	, refresh_on_start(false)
	, api(0)
	, reloader(this)
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
	::signal(SIGCHLD, omg_a_child_died);

	if (!configpaths.initialized()) {
		std::cerr << configpaths.error_message() << std::endl;
		return EXIT_FAILURE;
	}

	refresh_on_start = args.refresh_on_start;

	if (args.set_log_file) {
		Logger::getInstance().set_logfile(args.log_file);
	}

	if (args.set_log_level) {
		Logger::getInstance().set_loglevel(args.log_level);
	}

	if (!args.display_msg.empty()) {
		std::cerr << args.display_msg << std::endl;
	}

	if (args.should_return) {
		return args.return_code;
	}

	configpaths.process_args(args);

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

	if (!configpaths.setup_dirs()) {
		return EXIT_FAILURE;
	}

	if (!args.do_export) {
		if (!args.silent)
			std::cout << StrPrintf::fmt(_("Starting %s %s..."),
					     PROGRAM_NAME,
					     PROGRAM_VERSION)
				  << std::endl;

		fslock = std::unique_ptr<FSLock>(new FSLock());
		pid_t pid;
		if (!fslock->try_lock(configpaths.lock_file(), pid)) {
			if (!args.execute_cmds) {
				std::cout << StrPrintf::fmt(
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

	Keymap keys(KM_NEWSBOAT);
	cfgparser.register_handler("bind-key", &keys);
	cfgparser.register_handler("unbind-key", &keys);
	cfgparser.register_handler("macro", &keys);

	cfgparser.register_handler("ignore-article", &ign);
	cfgparser.register_handler("always-Download", &ign);
	cfgparser.register_handler("reset-unread-on-update", &ign);

	cfgparser.register_handler("define-filter", &filters);
	cfgparser.register_handler("highlight", &rxman);
	cfgparser.register_handler("highlight-article", &rxman);

	try {
		cfgparser.parse("/etc/" PROGRAM_NAME "/config");
		cfgparser.parse(configpaths.config_file());
	} catch (const ConfigException& ex) {
		LOG(Level::ERROR,
			"an Exception occurred while parsing the configuration "
			"file: %s",
			ex.what());
		std::cout << ex.what() << std::endl;
		return EXIT_FAILURE;
	}

	update_config();

	if (!args.silent)
		std::cout << _("done.") << std::endl;

	// create Cache object
	std::string Cachefilepath = cfg.get_configvalue("Cache-file");
	if (Cachefilepath.length() > 0 && !args.set_cache_file) {
		configpaths.set_cache_file(Cachefilepath);
		fslock = std::unique_ptr<FSLock>(new FSLock());
		pid_t pid;
		if (!fslock->try_lock(configpaths.lock_file(), pid)) {
			std::cout << StrPrintf::fmt(
					     _("Error: an instance of %s is "
					       "already running (PID: %u)"),
					     PROGRAM_NAME,
					     pid)
				  << std::endl;
			return EXIT_FAILURE;
		}
	}

	if (!args.silent) {
		std::cout << _("Opening Cache...");
		std::cout.flush();
	}
	try {
		rsscache = new Cache(configpaths.cache_file(), &cfg);
	} catch (const DbException& e) {
		std::cerr << StrPrintf::fmt(
				     _("Error: opening the Cache file `%s' "
				       "failed: %s"),
				     configpaths.cache_file(),
				     e.what())
			  << std::endl;
		return EXIT_FAILURE;
	} catch (const std::runtime_error& e) {
		std::cerr << StrPrintf::fmt(
				     _("Error: opening the Cache file `%s' "
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
		urlcfg = new FileUrlReader(configpaths.url_file());
	} else if (type == "opml") {
		urlcfg = new OpmlUrlReader(&cfg);
	} else if (type == "oldreader") {
		api = new OldReaderApi(&cfg);
		urlcfg = new OldReaderUrlReader(
			&cfg, configpaths.url_file(), api);
	} else if (type == "ttrss") {
		api = new TtrssApi(&cfg);
		urlcfg = new TtrssUrlReader(configpaths.url_file(), api);
	} else if (type == "NewsBlur") {
		const auto cookies = cfg.get_configvalue("cookie-Cache");
		if (cookies.empty()) {
			std::cout << StrPrintf::fmt(
				_("ERROR: You must set `cookie-Cache` to use "
				  "Newsblur.\n"));
			return EXIT_FAILURE;
		}

		std::ofstream check(cookies);
		if (!check.is_open()) {
			std::cout << StrPrintf::fmt(
				_("%s is inaccessible and can't be created\n"),
				cookies);
			return EXIT_FAILURE;
		}

		api = new NewsBlurApi(&cfg);
		urlcfg = new NewsBlurUrlReader(configpaths.url_file(), api);
	} else if (type == "feedhq") {
		api = new FeedHqApi(&cfg);
		urlcfg =
			new FeedHqUrlReader(&cfg, configpaths.url_file(), api);
	} else if (type == "ocnews") {
		api = new OcNewsApi(&cfg);
		urlcfg = new OcNewsUrlReader(configpaths.url_file(), api);
	} else if (type == "inoreader") {
		api = new InoReaderApi(&cfg);
		urlcfg = new InoReaderUrlReader(
			&cfg, configpaths.url_file(), api);
	} else {
		LOG(Level::ERROR,
			"unknown urls-source `%s'",
			urlcfg->get_source());
	}

	if (!args.do_export && !args.silent) {
		std::cout << StrPrintf::fmt(
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
			msg = StrPrintf::fmt(
				_("Error: no URLs configured. Please fill the "
				  "file %s with RSS feed URLs or import an "
				  "OPML file."),
				configpaths.url_file());
		} else if (type == "opml") {
			msg = StrPrintf::fmt(
				_("It looks like the OPML feed you subscribed "
				  "contains no feeds. Please fill it with "
				  "feeds, and try again."));
		} else if (type == "oldreader") {
			msg = StrPrintf::fmt(
				_("It looks like you haven't configured any "
				  "feeds in your The Old Reader account. "
				  "Please do so, and try again."));
		} else if (type == "ttrss") {
			msg = StrPrintf::fmt(
				_("It looks like you haven't configured any "
				  "feeds in your Tiny Tiny RSS account. Please "
				  "do so, and try again."));
		} else if (type == "NewsBlur") {
			msg = StrPrintf::fmt(
				_("It looks like you haven't configured any "
				  "feeds in your NewsBlur account. Please do "
				  "so, and try again."));
		} else if (type == "inoreader") {
			msg = StrPrintf::fmt(
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
		std::cout << _("Loading articles from Cache...");
	if (args.do_vacuum)
		std::cout << _("Opening Cache...");
	std::cout.flush();

	if (args.do_vacuum) {
		std::cout << _("done.") << std::endl;
		std::cout << _("Cleaning up Cache thoroughly...");
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
			std::cout << StrPrintf::fmt(
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

	Formaction::load_histories(
		configpaths.search_file(), configpaths.cmdline_file());

	// run the View
	int ret = v->run();

	unsigned int History_limit =
		cfg.get_configvalue_as_int("History-limit");
	LOG(Level::DEBUG, "Controller::run: History-limit = %u", History_limit);
	Formaction::save_histories(configpaths.search_file(),
		configpaths.cmdline_file(),
		History_limit);

	if (!args.silent) {
		std::cout << _("Cleaning up Cache...");
		std::cout.flush();
	}
	try {
		std::lock_guard<std::mutex> feedslock(feeds_mutex);
		rsscache->cleanup_cache(feedcontainer.feeds);
		if (!args.silent) {
			std::cout << _("done.") << std::endl;
		}
	} catch (const DbException& e) {
		LOG(Level::USERERROR, "Cleaning up Cache failed: %s", e.what());
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
		v->show_error(StrPrintf::fmt(
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
	enqueue_items(feed);

	oldfeed->clear_items();

	v->notify_itemlist_change(feedcontainer.feeds[pos]);
	if (!unattended) {
		v->set_feedlist(feedcontainer.feeds);
	}
}

void Controller::import_opml(const std::string& filename)
{
	if (!OPML::import(filename, urlcfg)) {
		std::cout << StrPrintf::fmt(
				     _("An error occurred while parsing %s."),
				     filename)
			  << std::endl;
		return;
	} else {
		std::cout << StrPrintf::fmt(_("Import of %s finished."), filename)
			  << std::endl;
	}
}

void Controller::export_opml()
{
	xmlDocPtr root = OPML::generate_opml(feedcontainer);

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

void Controller::enqueue_url(const std::string& url,
	const std::string& title,
	const time_t pubDate,
	std::shared_ptr<RssFeed> feed)
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
					Utils::tokenize_quoted(line);
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
		std::string filename =
			generate_enqueue_filename(url, title, pubDate, feed);
		f << url << " " << Stfl::quote(filename) << std::endl;
		f.close();
	}
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
					"Exception: %s",
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

	std::string cmdline = StrPrintf::fmt("%s \"%s\"",
		editor,
		Utils::replace_all(configpaths.url_file(), "\"", "\\\""));

	v->push_empty_formaction();
	Stfl::reset();

	Utils::run_interactively(cmdline, "Controller::edit_urls_file");

	v->pop_current_formaction();

	reload_urls_file();
}

int Controller::execute_commands(const std::vector<std::string>& cmds)
{
	if (v->Formaction_stack_size() > 0)
		v->pop_current_formaction();
	for (const auto& cmd : cmds) {
		LOG(Level::DEBUG,
			"Controller::execute_commands: executing `%s'",
			cmd);
		if (cmd == "reload") {
			reloader.reload_all(true);
		} else if (cmd == "print-unread") {
			std::cout << StrPrintf::fmt(_("%u unread articles"),
					     rsscache->get_unread_count())
				  << std::endl;
		} else {
			std::cerr
				<< StrPrintf::fmt(_("%s: %s: unknown command"),
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
	char *tmpdir = getenv("TMPDIR");
	if (tmpdir != nullptr) {
	  snprintf(filename, sizeof(filename), "%s/newsboat-article.XXXXXX", tmpdir);
	} else {
	  snprintf(filename, sizeof(filename), "/tmp/newsboat-article.XXXXXX");
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

	HtmlRenderer rnd(true);
	rnd.render(item->description(), lines, links, item->feedurl());
	TextFormatter txtfmt;
	txtfmt.add_lines(lines);

	unsigned int width = cfg.get_configvalue_as_int("text-width");
	if (width == 0)
		width = 80;
	ostr << txtfmt.format_text_plain(width) << std::endl;
}

void Controller::enqueue_items(std::shared_ptr<RssFeed> feed)
{
	if (!cfg.get_configvalue_as_bool("podcast-auto-enqueue"))
		return;
	std::lock_guard<std::mutex> lock(feed->item_mutex);
	for (const auto& item : feed->items()) {
		if (!item->enqueued() && item->enclosure_url().length() > 0) {
			LOG(Level::DEBUG,
				"Controller::enqueue_items: enclosure_url = "
				"`%s' "
				"enclosure_type = `%s'",
				item->enclosure_url(),
				item->enclosure_type());
			if (Utils::is_http_url(item->enclosure_url())) {
				LOG(Level::INFO,
					"Controller::enqueue_items: enqueuing "
					"`%s'",
					item->enclosure_url());
				enqueue_url(item->enclosure_url(),
					item->title(),
					item->pubDate_timestamp(),
					feed);
				item->set_enqueued(true);
				rsscache->update_rssitem_unread_and_enqueued(
					item, feed->rssurl());
			}
		}
	}
}

std::string Controller::generate_enqueue_filename(const std::string& url,
	const std::string& title,
	const time_t pubDate,
	std::shared_ptr<RssFeed> feed)
{
	std::string dlformat = cfg.get_configvalue("download-path");
	if (dlformat[dlformat.length() - 1] != NEWSBEUTER_PATH_SEP[0])
		dlformat.append(NEWSBEUTER_PATH_SEP);

	std::string filemask = cfg.get_configvalue("download-filename-format");
	dlformat.append(filemask);

	auto time_formatter = [&pubDate](const char* format) {
		char pubDate_formatted[1024];
		strftime(pubDate_formatted,
			sizeof(pubDate_formatted),
			format,
			localtime(&pubDate));
		return std::string(pubDate_formatted);
	};

	std::string base = Utils::get_basename(url);
	std::string extension;
	std::size_t pos = base.rfind('.');
	if (pos != std::string::npos) {
		extension.append(base.substr(pos + 1));
	}

	FmtStrFormatter fmt;
	fmt.register_fmt('n', feed->title());
	fmt.register_fmt('h', get_hostname_from_url(url));
	fmt.register_fmt('u', base);
	fmt.register_fmt('F', time_formatter("%F"));
	fmt.register_fmt('m', time_formatter("%m"));
	fmt.register_fmt('b', time_formatter("%b"));
	fmt.register_fmt('d', time_formatter("%d"));
	fmt.register_fmt('H', time_formatter("%H"));
	fmt.register_fmt('M', time_formatter("%M"));
	fmt.register_fmt('S', time_formatter("%S"));
	fmt.register_fmt('y', time_formatter("%y"));
	fmt.register_fmt('Y', time_formatter("%Y"));
	fmt.register_fmt('t', title);
	fmt.register_fmt('e', extension);

	std::string dlpath = fmt.do_format(dlformat);
	return dlpath;
}

std::string Controller::get_hostname_from_url(const std::string& url)
{
	xmlURIPtr uri = xmlParseURI(url.c_str());
	std::string hostname;
	if (uri) {
		hostname = uri->server;
		xmlFreeURI(uri);
	}
	return hostname;
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
	v->set_RegexManager(&rxman);
	v->update_bindings();

	if (colorman.colors_loaded()) {
		v->set_colors(colorman.get_fgcolors(),
			colorman.get_bgcolors(),
			colorman.get_attributes());
		v->apply_colors_to_all_formactions();
	}

	if (cfg.get_configvalue("error-log").length() > 0) {
		try {
			Logger::getInstance().set_errorlogfile(
				cfg.get_configvalue("error-log"));
		} catch (const Exception& e) {
			const std::string msg =
				StrPrintf::fmt("Couldn't open %s: %s",
					cfg.get_configvalue("error-log"),
					e.what());
			v->show_error(msg);
			std::cerr << msg << std::endl;
		}
	}
}

void Controller::load_configfile(const std::string& filename)
{
	if (cfgparser.parse(filename, true)) {
		update_config();
	} else {
		v->show_error(StrPrintf::fmt(
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

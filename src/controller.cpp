#define ENABLE_IMPLICIT_FILEPATH_CONVERSIONS

#include "controller.h"

#include <cassert>
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <curl/curl.h>
#include <fstream>
#include <iostream>
#include <langinfo.h>
#include <libgen.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xmlsave.h>
#include <libxml/xmlversion.h>
#include <memory>
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
#include "configexception.h"
#include "configpaths.h"
#include "dbexception.h"
#include "exception.h"
#include "feedhqapi.h"
#include "feedhqurlreader.h"
#include "formaction.h"
#include "feedbinapi.h"
#include "freshrssapi.h"
#include "freshrssurlreader.h"
#include "fileurlreader.h"
#include "inoreaderapi.h"
#include "inoreaderurlreader.h"
#include "itemrenderer.h"
#include "logger.h"
#include "minifluxapi.h"
#include "minifluxurlreader.h"
#include "newsblurapi.h"
#include "ocnewsapi.h"
#include "oldreaderapi.h"
#include "oldreaderurlreader.h"
#include "opml.h"
#include "opmlurlreader.h"
#include "regexmanager.h"
#include "remoteapi.h"
#include "remoteapiurlreader.h"
#include "rssfeed.h"
#include "rssparser.h"
#include "scopemeasure.h"
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

Controller::Controller(ConfigPaths& configpaths)
	: v(0)
	, refresh_on_start(false)
	, configpaths(configpaths)
	, queueManager(&cfg, configpaths.queue_file())
{
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

	refresh_on_start = args.refresh_on_start();

	if (args.log_level().has_value()) {
		logger::set_loglevel(args.log_level().value());
	}

	if (args.log_file().has_value()) {
		logger::set_logfile(args.log_file().value());
	}

	if (!args.log_file().has_value() && args.log_level().has_value()) {
		const std::string date_time_string = utils::mt_strf_localtime("%Y-%m-%d_%H.%M.%S",
				std::time(nullptr));
		const std::string filename = "newsboat_" + date_time_string + ".log";
		const auto filepath = Filepath::from_locale_string(filename);
		logger::set_logfile(filepath);
	}

	if (!args.display_msg().empty()) {
		std::cerr << args.display_msg() << std::endl;
	}

	if (args.return_code().has_value()) {
		return args.return_code().value();
	}

	const auto migrated = configpaths.try_migrate_from_newsbeuter();
	if (migrated) {
		std::cerr << "\nPlease check the results and press Enter to "
			"continue.";
		std::cin.ignore();
	}

	if (!configpaths.create_dirs()) {
		return EXIT_FAILURE;
	}

	if (args.do_import()) {
		LOG(Level::INFO, "Importing OPML file from %s", args.importfile());
		return import_opml(args.importfile(), configpaths.url_file());
	}

	LOG(Level::INFO, "nl_langinfo(CODESET): %s", nl_langinfo(CODESET));

	if (!args.do_export()) {
		if (!args.silent())
			std::cout << strprintf::fmt(_("Starting %s %s..."),
					PROGRAM_NAME,
					utils::program_version())
				<< std::endl;
	}

	if (!args.silent()) {
		std::cout << _("Loading configuration...");
	}
	std::cout.flush();

	cfg.register_commands(cfgparser);
	colorman.register_commands(cfgparser);

	KeyMap keys(KM_NEWSBOAT);
	cfgparser.register_handler("bind", keys);
	cfgparser.register_handler("bind-key", keys);
	cfgparser.register_handler("unbind-key", keys);
	cfgparser.register_handler("macro", keys);
	cfgparser.register_handler("run-on-startup", keys);

	cfgparser.register_handler("ignore-article", ign);
	cfgparser.register_handler("always-download", ign);
	cfgparser.register_handler("reset-unread-on-update", ign);

	cfgparser.register_handler("define-filter", filters);
	cfgparser.register_handler("highlight", rxman);
	cfgparser.register_handler("highlight-article", rxman);
	cfgparser.register_handler("highlight-feed", rxman);

	try {
		cfgparser.parse_file("/etc/" PACKAGE "/config");
		cfgparser.parse_file(configpaths.config_file());
	} catch (const ConfigException& ex) {
		LOG(Level::ERROR,
			"an exception occurred while parsing the configuration "
			"file: %s",
			ex.what());
		std::cout << ex.what() << std::endl;
		return EXIT_FAILURE;
	}

	update_config();

	if (!args.silent()) {
		std::cout << _("done.") << std::endl;
	}

	// create cache object
	std::string cachefilepath = cfg.get_configvalue("cache-file");
	if (cachefilepath.length() > 0 && !args.cache_file().has_value()) {
		configpaths.set_cache_file(cachefilepath);
	}

	pid_t pid;
	std::string error;
	if (!fslock.try_lock(configpaths.lock_file(), pid, error)) {
		if (pid != 0) {
			std::cout << strprintf::fmt(
					_("Error: an instance of %s is "
						"already running (PID: %s)"),
					PROGRAM_NAME,
					std::to_string(pid))
				<< std::endl;
		} else {
			std::cout << _("Error: ") << error << std::endl;
		}
		return EXIT_FAILURE;
	}

	if (!args.silent()) {
		std::cout << _("Opening cache...");
		std::cout.flush();
	}
	try {
		rsscache = std::make_unique<Cache>(configpaths.cache_file(), &cfg);
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

	if (!args.silent()) {
		std::cout << _("done.") << std::endl;
	}

	reloader = std::make_unique<Reloader>(*this, *rsscache, cfg);

	std::string type = cfg.get_configvalue("urls-source");
	if (type == "local") {
		urlcfg = std::make_unique<FileUrlReader>(configpaths.url_file());
	} else if (type == "opml") {
		urlcfg = std::make_unique<OpmlUrlReader>(cfg, configpaths.url_file());
	} else if (type == "oldreader") {
		api = std::make_unique<OldReaderApi>(cfg);
		urlcfg = std::make_unique<OldReaderUrlReader>(
				&cfg, configpaths.url_file(), api.get());
	} else if (type == "ttrss") {
		api = std::make_unique<TtRssApi>(cfg);
		urlcfg = std::make_unique<RemoteApiUrlReader>("Tiny Tiny RSS", configpaths.url_file(),
				*api);
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

		api = std::make_unique<NewsBlurApi>(cfg);
		urlcfg = std::make_unique<RemoteApiUrlReader>("NewsBlur", configpaths.url_file(), *api);
	} else if (type == "feedhq") {
		api = std::make_unique<FeedHqApi>(cfg);
		urlcfg = std::make_unique<FeedHqUrlReader>(&cfg, configpaths.url_file(), api.get());
	} else if (type == "feedbin") {
		const std::string user = cfg.get_configvalue("feedbin-login");
		const std::string pass = cfg.get_configvalue("feedbin-password");
		const std::string pass_file = cfg.get_configvalue("feedbin-passwordfile");
		const std::string pass_eval = cfg.get_configvalue("feedbin-passwordeval");
		const bool creds_set = !user.empty() &&
			(!pass.empty() || !pass_file.empty() || !pass_eval.empty());
		if (!creds_set) {
			std::cerr <<
				_("ERROR: You must set `feedbin-login` and one of `feedbin-password`, "
					"`feedbin-passwordfile` or `feedbin-passwordeval` to use "
					"Feedbin\n");
			return EXIT_FAILURE;
		}

		api = std::make_unique<FeedbinApi>(cfg);
		urlcfg = std::make_unique<RemoteApiUrlReader>("Feedbin", configpaths.url_file(), *api);
	} else if (type == "freshrss") {
		const auto freshrss_url = cfg.get_configvalue("freshrss-url");
		if (freshrss_url.empty()) {
			std::cerr <<
				_("ERROR: You must set `freshrss-url` to use FreshRSS\n");
			return EXIT_FAILURE;
		}

		const std::string user = cfg.get_configvalue("freshrss-login");
		const std::string pass = cfg.get_configvalue("freshrss-password");
		const std::string pass_file = cfg.get_configvalue("freshrss-passwordfile");
		const std::string pass_eval = cfg.get_configvalue("freshrss-passwordeval");
		const bool creds_set = !user.empty() &&
			(!pass.empty() || !pass_file.empty() || !pass_eval.empty());
		if (!creds_set) {
			std::cerr <<
				_("ERROR: You must set `freshrss-login` and one of `freshrss-password`, "
					"`freshrss-passwordfile` or `freshrss-passwordeval` to use "
					"FreshRSS\n");
			return EXIT_FAILURE;
		}

		api = std::make_unique<FreshRssApi>(cfg);
		urlcfg = std::make_unique<FreshRssUrlReader>(&cfg, configpaths.url_file(), api.get());
	} else if (type == "ocnews") {
		api = std::make_unique<OcNewsApi>(cfg);
		urlcfg = std::make_unique<RemoteApiUrlReader>("ownCloud News", configpaths.url_file(),
				*api);
	} else if (type == "miniflux") {
		const auto miniflux_url = cfg.get_configvalue("miniflux-url");
		if (miniflux_url.empty()) {
			std::cerr <<
				_("ERROR: You must set `miniflux-url` to use Miniflux\n");
			return EXIT_FAILURE;
		}

		const std::string user = cfg.get_configvalue("miniflux-login");
		const std::string pass = cfg.get_configvalue("miniflux-password");
		const std::string pass_file = cfg.get_configvalue("miniflux-passwordfile");
		const std::string pass_eval = cfg.get_configvalue("miniflux-passwordeval");
		const std::string token = cfg.get_configvalue("miniflux-token");
		const std::string token_file = cfg.get_configvalue("miniflux-tokenfile");
		const std::string token_eval = cfg.get_configvalue("miniflux-tokeneval");
		const bool creds_set = !token.empty()
			|| !token_file.empty()
			|| !token_eval.empty()
			|| (!user.empty() && (!pass.empty() || !pass_file.empty() || !pass_eval.empty()));
		if (!creds_set) {
			std::cerr <<
				_("ERROR: You must provide an API token or a login/password pair to use Miniflux. Please set the appropriate miniflux-* settings\n");
			return EXIT_FAILURE;
		}

		api = std::make_unique<MinifluxApi>(cfg);
		urlcfg = std::make_unique<MinifluxUrlReader>(&cfg, configpaths.url_file(), api.get());
	} else if (type == "inoreader") {
		const auto all_set = !cfg.get_configvalue("inoreader-app-id").empty()
			&& !cfg.get_configvalue("inoreader-app-key").empty();
		if (!all_set) {
			std::cerr <<
				_("ERROR: You must set *both* `inoreader-app-id` and `inoreader-app-key` to use Inoreader.\n");
			return EXIT_FAILURE;
		}

		api = std::make_unique<InoreaderApi>(cfg);
		urlcfg = std::make_unique<InoreaderUrlReader>(&cfg, configpaths.url_file(), api.get());
	} else {
		std::cerr << strprintf::fmt(_("ERROR: Unknown urls-source `%s'"),
				type) << std::endl;
		return EXIT_FAILURE;
	}

	if (!args.do_export() && !args.silent()) {
		std::cout << strprintf::fmt(
				_("Loading URLs from %s..."), urlcfg->get_source());
		std::cout.flush();
	}
	if (api) {
		if (!api->authenticate()) {
			std::cerr << _("Authentication failed.") << std::endl;
			return EXIT_FAILURE;
		}
	}
	const auto error_message = urlcfg->reload();
	if (error_message.has_value()) {
		std::cout << error_message.value().message << std::endl << std::endl;
	} else if (!args.do_export() && !args.silent()) {
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
		} else if (type == "miniflux") {
			msg = strprintf::fmt(
					_("It looks like you haven't configured any "
						"feeds in your Miniflux account. Please do "
						"so, and try again."));
		} else if (type == "feedbin") {
			msg = strprintf::fmt(
					_("It looks like you haven't configured any "
						"feeds in your Feedbin account. Please do "
						"so, and try again."));
		} else if (type == "ocnews") {
			msg = strprintf::fmt(
					_("It looks like you haven't configured any feeds in your "
						"Owncloud/Nextcloud account. Please do so, and try "
						"again."));
		} else {
			assert(0); // shouldn't happen
		}
		std::cout << msg << std::endl << std::endl;
		return EXIT_FAILURE;
	}

	if (args.do_vacuum()) {
		std::cout << _("Opening cache...");
		std::cout << _("done.") << std::endl;
		std::cout << _("Cleaning up cache thoroughly...");
		std::cout.flush();
		rsscache->do_vacuum();
		std::cout << _("done.") << std::endl;
		return EXIT_SUCCESS;
	}

	if (!args.do_export() && !args.silent()) {
		std::cout << _("Loading articles from cache...");
	}
	std::cout.flush();

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

	if (!args.do_export() && !args.silent()) {
		std::cout << _("done.") << std::endl;
	}

	if (args.do_cleanup()) {
		std::cout << _("Cleaning up cache...");
		std::cout.flush();
		rsscache->cleanup_cache(feedcontainer.get_all_feeds(), true);
		std::cout << _("done.") << std::endl;
		return EXIT_SUCCESS;
	}

	// if configured, we fill all query feeds with some data; no need to
	// sort it, it will be refilled when actually opening it.
	if (cfg.get_configvalue_as_bool("prepopulate-query-feeds")) {
		if (!args.do_export() && !args.silent()) {
			std::cout << _("Prepopulating query feeds...");
			std::cout.flush();
		}

		feedcontainer.populate_query_feeds();

		if (!args.do_export() && !args.silent()) {
			std::cout << _("done.") << std::endl;
		}
	}

	feedcontainer.sort_feeds(cfg.get_feed_sort_strategy());

	if (args.do_export()) {
		export_opml(args.export_as_opml2());
		return EXIT_SUCCESS;
	}

	if (args.readinfo_import_file().has_value()) {
		LOG(Level::INFO,
			"Importing read information file from %s",
			args.readinfo_import_file().value());
		std::cout << _("Importing list of read articles...");
		std::cout.flush();
		import_read_information(args.readinfo_import_file().value());
		std::cout << _("done.") << std::endl;
		return EXIT_SUCCESS;
	}

	if (args.readinfo_export_file().has_value()) {
		LOG(Level::INFO,
			"Exporting read information file to %s",
			args.readinfo_export_file().value());
		std::cout << _("Exporting list of read articles...");
		std::cout.flush();
		export_read_information(args.readinfo_export_file().value());
		std::cout << _("done.") << std::endl;
		return EXIT_SUCCESS;
	}

	// hand over the important objects to the View
	v->set_config_container(&cfg);
	v->set_keymap(&keys);
	v->set_tags(urlcfg->get_alltags());
	v->set_cache(rsscache.get());

	const auto cmds_to_execute = args.cmds_to_execute();
	if (cmds_to_execute.size() >= 1) {
		execute_commands(cmds_to_execute);
		return EXIT_SUCCESS;
	}

	// if the user wants to refresh on startup via configuration file, then
	// do so, but only if -r hasn't been supplied.
	if (!refresh_on_start &&
		cfg.get_configvalue_as_bool("refresh-on-startup")) {
		refresh_on_start = true;
	}

	FormAction::load_histories(
		configpaths.search_history_file(), configpaths.cmdline_history_file());

	// run the View
	int ret = v->run();

	unsigned int history_limit =
		cfg.get_configvalue_as_int("history-limit");
	LOG(Level::DEBUG, "Controller::run: history-limit = %u", history_limit);
	FormAction::save_histories(configpaths.search_history_file(),
		configpaths.cmdline_history_file(),
		history_limit);

	if (!args.silent()) {
		std::cout << _("Cleaning up cache...");
		std::cout.flush();
	}
	try {
		const auto unreachable_feeds = rsscache->cleanup_cache(
				feedcontainer.get_all_feeds());
		if (!args.silent()) {
			std::cout << _("done.") << std::endl;
			if (!unreachable_feeds.empty()) {
				for (const auto& feed : unreachable_feeds) {
					LOG(Level::USERERROR, "Unreachable feed found: %s", feed);
				}

				// Workaround for missing overload of strprintf::fmt for size_type on macOS.
				std::uint64_t num_feeds = unreachable_feeds.size();
				std::cout << strprintf::fmt(_("%" PRIu64 " unreachable feeds found. See "
							"`cleanup-on-quit` in newsboat(1) for details."), num_feeds)
					<< std::endl;
				if (cfg.get_configvalue("cleanup-on-quit") == "nudge") {
					std::cout << _("Press any key to continue") << std::endl;
					utils::wait_for_keypress();
				}
			}
		}
	} catch (const DbException& e) {
		LOG(Level::USERERROR, "Cleaning up cache failed: %s", e.what());
		if (!args.silent()) {
			std::cout << _("failed: ") << e.what() << std::endl;
			ret = EXIT_FAILURE;
		}
	}

	return ret;
}

void Controller::update_feedlist()
{
	v->set_feedlist(feedcontainer.get_all_feeds());
}

void Controller::update_visible_feeds()
{
	v->update_visible_feeds(feedcontainer.get_all_feeds());
}

void Controller::mark_all_read(const std::string& feedurl)
{
	try {
		rsscache->mark_all_read(feedurl);
	} catch (const DbException& e) {
		v->get_statusline().show_error(strprintf::fmt(
				_("Error: couldn't mark all feeds read: %s"),
				e.what()));
		return;
	}

	if (feedurl.empty()) { // Mark all feeds as read
		if (api) {
			for (const auto& feed : feedcontainer.get_all_feeds()) {
				api->mark_all_read(feed->rssurl());
			}
		}
		feedcontainer.mark_all_feeds_read();
	} else { // Mark a specific feed as read
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
	ScopeMeasure m("Controller::mark_all_read");
	const auto feed = feedcontainer.get_feed(pos);
	if (feed == nullptr) {
		return;
	}

	if (feed->is_query_feed()) {
		if (api) {
			std::vector<std::string> item_guids;
			for (const auto& item : feed->items()) {
				if (item->unread()) {
					item_guids.push_back(item->guid());
				}
			}
			api->mark_articles_read(item_guids);
		}
		rsscache->mark_all_read(*feed);
	} else {
		rsscache->mark_all_read(feed->rssurl());
		if (api) {
			api->mark_all_read(feed->rssurl());
		}
	}
	m.stopover(
		"after rsscache->mark_all_read, before iteration over "
		"items");

	feedcontainer.mark_all_feed_items_read(feed);
}

void Controller::mark_all_read(const std::vector<std::string>& item_guids)
{
	ScopeMeasure m("Controller::mark_all_read");
	if (api) {
		api->mark_articles_read(item_guids);
	}
}

void Controller::replace_feed(RssFeed& oldfeed, RssFeed& newfeed, unsigned int pos,
	bool unattended)
{
	LOG(Level::DEBUG, "Controller::replace_feed: saving");
	rsscache->externalize_rssfeed(
		newfeed, ign.matches_resetunread(newfeed.rssurl()));
	LOG(Level::DEBUG,
		"Controller::replace_feed: after externalize_rssfeed");

	bool ignore_disp = (cfg.get_configvalue("ignore-mode") == "display");
	std::shared_ptr<RssFeed> feed = rsscache->internalize_rssfeed(
			oldfeed.rssurl(), ignore_disp ? &ign : nullptr);
	LOG(Level::DEBUG,
		"Controller::replace_feed: after internalize_rssfeed");

	feed->set_tags(urlcfg->get_tags(oldfeed.rssurl()));
	feed->set_order(oldfeed.get_order());
	feedcontainer.replace_feed(pos, feed);

	if (cfg.get_configvalue_as_bool("podcast-auto-enqueue")) {
		const auto result = queueManager.autoenqueue(*feed);
		switch (result.status) {
		case EnqueueStatus::QUEUED_SUCCESSFULLY:
		case EnqueueStatus::URL_QUEUED_ALREADY:
			// All is well, nothing to be done
			break;
		case EnqueueStatus::OUTPUT_FILENAME_USED_ALREADY:
			v->get_statusline().show_error(
				strprintf::fmt(_("Generated filename (%s) is used already."),
					result.extra_info));
			break;
		case EnqueueStatus::QUEUE_FILE_OPEN_ERROR:
			v->get_statusline().show_error(
				strprintf::fmt(_("Failed to open queue file: %s."), result.extra_info));
			break;
		}
	}

	for (const auto& item : feed->items()) {
		rsscache->update_rssitem_unread_and_enqueued(*item, feed->rssurl());
	}

	v->notify_itemlist_change(feed);
	if (!unattended) {
		v->set_feedlist(feedcontainer.get_all_feeds());
	}
}

int Controller::import_opml(const Filepath& opmlFile,
	const Filepath& urlFile)
{
	FileUrlReader urlReader(urlFile);
	const auto error_message = urlReader.reload(); // Load existing URLs
	if (error_message.has_value()) {
		std::cout << strprintf::fmt(
				_("Error importing OPML to urls file %s: %s"),
				urlFile, error_message.value().message)
			<< std::endl;
		return EXIT_FAILURE;
	}

	const auto import_error = opml::import(opmlFile, urlReader);
	if (import_error.has_value()) {
		std::cout << strprintf::fmt(
				_("An error occurred while parsing %s: %s"),
				opmlFile,
				import_error.value())
			<< std::endl;
		return EXIT_FAILURE;
	} else {
		std::cout << strprintf::fmt(
				_("Import of %s finished."), opmlFile)
			<< std::endl;
		return EXIT_SUCCESS;
	}
}

void Controller::export_opml(bool version2)
{
	xmlDocPtr root = opml::generate(feedcontainer, version2);

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
	if (feed && (feed->is_query_feed() || feed->is_search_feed())) {
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
				query, (feed != nullptr ? feed->rssurl() : ""), ign);
		for (const auto& item : items) {
			item->set_feedptr(
				feedcontainer.get_feed_by_url(item->feedurl()));
		}
	}
	return items;
}

EnqueueResult Controller::enqueue_url(RssItem& item, RssFeed& feed)
{
	return queueManager.enqueue_url(item, feed);
}

void Controller::reload_urls_file()
{
	const auto error_message = urlcfg->reload();
	if (error_message.has_value()) {
		v->get_statusline().show_message(error_message.value().message);
		return;
	}

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
	if (!editor) {
		editor = getenv("EDITOR");
	}
	if (!editor) {
		editor = "vi";
	}

	std::string cmdline = strprintf::fmt("%s \"%s\"",
			editor,
			utils::replace_all(configpaths.url_file(), "\"", "\\\""));

	v->push_empty_formaction();
	Stfl::reset();
	utils::run_interactively(cmdline, "Controller::edit_urls_file");
	v->drop_queued_input();

	v->pop_current_formaction();

	reload_urls_file();
}

int Controller::execute_commands(const std::vector<std::string>& cmds)
{
	if (v->formaction_stack_size() > 0) {
		v->pop_current_formaction();
	}
	for (const auto& cmd : cmds) {
		LOG(Level::DEBUG,
			"Controller::execute_commands: executing `%s'",
			cmd);
		if (cmd == "reload") {
			reloader->reload_all(true);
		} else if (cmd == "print-unread") {
			std::cout << strprintf::fmt(_("%u unread articles"),
					feedcontainer.unread_item_count())
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

Filepath Controller::write_temporary_item(RssItem& item)
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

void Controller::write_item(RssItem& item, const Filepath& filename)
{
	const std::string save_path = cfg.get_configvalue("save-path");
	Filepath spath = save_path.back() == '/' ? save_path : save_path + "/";

	Filepath path;
	if (filename.starts_with("/")) {
		path.push(filename);
	} else if (filename.starts_with("~")) {
		path = utils::resolve_tilde(filename);
	} else {
		path = spath.join(filename);
	}

	std::fstream f(path, std::fstream::out);
	if (!f.is_open()) {
		throw Exception(errno);
	}

	write_item(item, f);
}

void Controller::write_item(RssItem& item, std::ostream& ostr)
{
	ostr << item_renderer::to_plain_text(cfg, item) << std::endl;
}

void Controller::import_read_information(const Filepath& readinfofile)
{
	std::vector<std::string> guids;

	std::ifstream f(readinfofile);
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

void Controller::export_read_information(const Filepath& readinfofile)
{
	std::vector<std::string> guids = rsscache->get_read_item_guids();

	std::fstream f(readinfofile, std::fstream::out);
	if (f.is_open()) {
		for (const auto& guid : guids) {
			f << guid << std::endl;
		}
	}
}

void Controller::update_config()
{
	v->apply_colors_to_all_formactions();

	if (cfg.get_configvalue("error-log").length() > 0) {
		try {
			const auto filepath = Filepath::from_locale_string(cfg.get_configvalue("error-log"));
			logger::set_user_error_logfile(filepath);
		} catch (const Exception& e) {
			const std::string msg =
				strprintf::fmt("Couldn't open %s: %s",
					cfg.get_configvalue("error-log"),
					e.what());
			v->get_statusline().show_error(msg);
			std::cerr << msg << std::endl;
		}
	}
}

void Controller::load_configfile(const Filepath& filename)
{
	if (cfgparser.parse_file(filename)) {
		update_config();
	} else {
		v->get_statusline().show_error(strprintf::fmt(
				_("Error: couldn't open configuration file `%s'!"),
				filename));
	}
}

void Controller::dump_config(const Filepath& filename) const
{
	std::vector<std::string> configlines;
	cfg.dump_config(configlines);
	if (v) {
		v->get_keymap()->dump_config(configlines);
	}
	ign.dump_config(configlines);
	filters.dump_config(configlines);
	colorman.dump_config(configlines);
	rxman.dump_config(configlines);
	std::fstream f(filename, std::fstream::out);
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

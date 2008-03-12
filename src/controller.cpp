#include <config.h>
#include <view.h>
#include <controller.h>
#include <configparser.h>
#include <configcontainer.h>
#include <exceptions.h>
#include <downloadthread.h>
#include <colormanager.h>
#include <logger.h>
#include <utils.h>
#include <stflpp.h>
#include <interpreter.h>
#include <sstream>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <fstream>
#include <cerrno>

#include <sys/time.h>
#include <ctime>
#include <cassert>
#include <signal.h>
#include <sys/utsname.h>

#include <_nxml.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <pwd.h>

#include <ncursesw/ncurses.h>

using namespace newsbeuter;

static std::string lock_file = "lock.pid";

void ctrl_c_action(int sig) {
	GetLogger().log(LOG_DEBUG,"caugh signal %d",sig);
	stfl::reset();
	::unlink(lock_file.c_str());
	if (SIGSEGV == sig) {
		fprintf(stderr,"%s\n", _("Segmentation fault."));
	}
	::exit(EXIT_FAILURE);
}

void ignore_signal(int sig) {
	GetLogger().log(LOG_WARN, "caught signal %d but ignored it", sig);
}

void omg_a_child_died(int /* sig */) {
	pid_t pid;
	int stat;
	while ((pid = waitpid(-1,&stat,WNOHANG)) > 0) { }
}

controller::controller() : v(0), rsscache(0), url_file("urls"), cache_file("cache.db"), config_file("config"), queue_file("queue"), refresh_on_start(false), cfg(0) {
	std::ostringstream cfgfile;

	char * cfgdir;
	if (!(cfgdir = ::getenv("HOME"))) {
		struct passwd * spw = ::getpwuid(::getuid());
		if (spw) {
			cfgdir = spw->pw_dir;
		} else {
			std::cout << _("Fatal error: couldn't determine home directory!") << std::endl;
			char buf[1024];
			snprintf(buf, sizeof(buf), _("Please set the HOME environment variable or add a valid user for UID %u!"), ::getuid());
			std::cout << buf << std::endl;
			::exit(EXIT_FAILURE);
		}
	}
	config_dir = cfgdir;


	config_dir.append(NEWSBEUTER_PATH_SEP);
	config_dir.append(NEWSBEUTER_CONFIG_SUBDIR);
	mkdir(config_dir.c_str(),0700); // create configuration directory if it doesn't exist

	url_file = config_dir + std::string(NEWSBEUTER_PATH_SEP) + url_file;
	cache_file = config_dir + std::string(NEWSBEUTER_PATH_SEP) + cache_file;
	config_file = config_dir + std::string(NEWSBEUTER_PATH_SEP) + config_file;
	lock_file = config_dir + std::string(NEWSBEUTER_PATH_SEP) + lock_file;
	queue_file = config_dir + std::string(NEWSBEUTER_PATH_SEP) + queue_file;
	reload_mutex = new mutex();
}

controller::~controller() {
	delete rsscache;
	delete reload_mutex;
	delete cfg;
}

void controller::set_view(view * vv) {
	v = vv;
}

void controller::run(int argc, char * argv[]) {
	int c;
	char msgbuf[1024];

	::signal(SIGINT, ctrl_c_action);
#ifndef DEBUG
	// ::signal(SIGSEGV, ctrl_c_action);
#endif
	::signal(SIGPIPE, ignore_signal);
	::signal(SIGHUP, ctrl_c_action);
	::signal(SIGCHLD, omg_a_child_died);

	bool do_import = false, do_export = false, cachefile_given_on_cmdline = false, do_vacuum = false;
	bool offline_mode = false, real_offline_mode = false;
	std::string importfile;

	do {
		if((c = ::getopt(argc,argv,"i:erhu:c:C:d:l:vVo"))<0)
			continue;
		switch (c) {
			case ':': /* fall-through */
			case '?': /* missing option */
				usage(argv[0]);
				break;
			case 'i':
				if (do_export)
					usage(argv[0]);
				do_import = true;
				importfile = optarg;
				break;
			case 'r':
				refresh_on_start = true;
				break;
			case 'e':
				if (do_import)
					usage(argv[0]);
				do_export = true;
				break;
			case 'h':
				usage(argv[0]);
				break;
			case 'u':
				url_file = optarg;
				break;
			case 'c':
				cache_file = optarg;
				cachefile_given_on_cmdline = true;
				break;
			case 'C':
				config_file = optarg;
				break;
			case 'v':
				do_vacuum = true;
				break;
			case 'V':
				version_information();
				break;
			case 'o':
				offline_mode = true;
				break;
			case 'd': // this is an undocumented debug commandline option!
				GetLogger().set_logfile(optarg);
				break;
			case 'l': // this is an undocumented debug commandline option!
				{
					loglevel level = static_cast<loglevel>(atoi(optarg));
					if (level > LOG_NONE && level <= LOG_DEBUG)
						GetLogger().set_loglevel(level);
				}
				break;
			default:
				snprintf(msgbuf, sizeof(msgbuf), _("%s: unknown option - %c"), argv[0], static_cast<char>(c));
				std::cout << msgbuf << std::endl;
				usage(argv[0]);
				break;
		}
	} while (c != -1);


	if (do_import) {
		GetLogger().log(LOG_INFO,"Importing OPML file from %s",importfile.c_str());
		urlcfg = new file_urlreader(url_file);
		import_opml(importfile.c_str());
		return;
	}


	if (!do_export) {
		snprintf(msgbuf, sizeof(msgbuf), _("Starting %s %s..."), PROGRAM_NAME, PROGRAM_VERSION);
		std::cout << msgbuf << std::endl;

		pid_t pid;
		if (!utils::try_fs_lock(lock_file, pid)) {
			if (pid > 0) {
				GetLogger().log(LOG_ERROR,"an instance is already running: pid = %u",pid);
			} else {
				GetLogger().log(LOG_ERROR,"something went wrong with the lock: %s", strerror(errno));
			}
			snprintf(msgbuf, sizeof(msgbuf), _("Error: an instance of %s is already running (PID: %u)"), PROGRAM_NAME, pid);
			std::cout << msgbuf << std::endl;
			return;
		}
	}

#if HAVE_RUBY
	GetInterpreter()->set_controller(this);
	GetInterpreter()->set_view(v);
#endif

	if (!do_export)
		std::cout << _("Loading configuration...");
	std::cout.flush();
	
	configparser cfgparser;
	cfg = new configcontainer();
	cfg->register_commands(cfgparser);
	colormanager * colorman = new colormanager();
	colorman->register_commands(cfgparser);

	keymap keys(KM_NEWSBEUTER);
	cfgparser.register_handler("bind-key",&keys);
	cfgparser.register_handler("unbind-key",&keys);
	cfgparser.register_handler("macro", &keys);

	cfgparser.register_handler("ignore-article",&ign);

	cfgparser.register_handler("define-filter",&filters);

#if HAVE_RUBY
	cfgparser.register_handler("load", GetInterpreter());
#endif

	try {
		cfgparser.parse("/etc/" PROGRAM_NAME "/config");
		cfgparser.parse(config_file);
	} catch (const configexception& ex) {
		GetLogger().log(LOG_ERROR,"an exception occured while parsing the configuration file: %s",ex.what());
		std::cout << ex.what() << std::endl;
		utils::remove_fs_lock(lock_file);
		return;	
	}


	if (colorman->colors_loaded()) {
		v->set_colors(*colorman);
	}
	delete colorman;

	if (cfg->get_configvalue("error-log").length() > 0) {
		GetLogger().set_errorlogfile(cfg->get_configvalue("error-log").c_str());
	}

	if (!do_export)
		std::cout << _("done.") << std::endl;

	// create cache object
	std::string cachefilepath = cfg->get_configvalue("cache-file");
	if (cachefilepath.length() > 0 && !cachefile_given_on_cmdline) {
		cache_file = cachefilepath.c_str();
	}

	if (!do_export) {
		std::cout << _("Opening cache...");
		std::cout.flush();
	}
	rsscache = new cache(cache_file,cfg);
	if (!do_export) {
		std::cout << _("done.") << std::endl;
	}


	std::string type = cfg->get_configvalue("urls-source");
	if (type == "local") {
		urlcfg = new file_urlreader(url_file);
	} else if (type == "bloglines") {
		urlcfg = new bloglines_urlreader(cfg);
		real_offline_mode = offline_mode;
	} else if (type == "opml") {
		urlcfg = new opml_urlreader(cfg);
		real_offline_mode = offline_mode;
	} else {
		GetLogger().log(LOG_ERROR,"unknown urls-source `%s'", urlcfg->get_source().c_str());
	}

	if (real_offline_mode) {
		if (!do_export) {
			snprintf(msgbuf,sizeof(msgbuf), _("Loading URLs from local cache..."));
			std::cout << msgbuf;
			std::cout.flush();
		}
		urlcfg->set_offline(true);
		urlcfg->get_urls() = rsscache->get_feed_urls();
		if (!do_export) {
			std::cout << _("done.") << std::endl;
		}
	} else {
		if (!do_export) {
			snprintf(msgbuf,sizeof(msgbuf), _("Loading URLs from %s..."), urlcfg->get_source().c_str());
			std::cout << msgbuf;
			std::cout.flush();
		}
		urlcfg->reload();
		if (!do_export) {
			std::cout << _("done.") << std::endl;
		}
	}

	if (urlcfg->get_urls().size() == 0) {
		GetLogger().log(LOG_ERROR,"no URLs configured.");
		if (type == "local") {
			snprintf(msgbuf, sizeof(msgbuf), _("Error: no URLs configured. Please fill the file %s with RSS feed URLs or import an OPML file."), url_file.c_str());
		} else if (type == "bloglines") {
			snprintf(msgbuf, sizeof(msgbuf), _("It looks like you haven't configured any feeds in your bloglines account. Please do so, and try again."));
		} else if (type == "opml") {
			snprintf(msgbuf, sizeof(msgbuf), _("It looks like the OPML feed you subscribed contains no feeds. Please fill it with feeds, and try again."));
		} else {
			assert(0); // shouldn't happen
		}
		std::cout << msgbuf << std::endl << std::endl;
		usage(argv[0]);
	}

	if (!do_export && !do_vacuum)
		std::cout << _("Loading articles from cache...");
	if (do_vacuum)
		std::cout << _("Opening cache...");
	std::cout.flush();


	if (do_vacuum) {
		std::cout << _("done.") << std::endl;
		std::cout << _("Cleaning up cache thoroughly...");
		std::cout.flush();
		rsscache->do_vacuum();
		std::cout << _("done.") << std::endl;
		utils::remove_fs_lock(lock_file);
		return;
	}

	for (std::vector<std::string>::const_iterator it=urlcfg->get_urls().begin(); it != urlcfg->get_urls().end(); ++it) {
		rss_feed feed(rsscache);
		feed.set_rssurl(*it);
		feed.set_tags(urlcfg->get_tags(*it));
		try {
			rsscache->internalize_rssfeed(feed);
		} catch(const dbexception& e) {
			std::cout << _("Error while loading feeds from database: ") << e.what() << std::endl;
			utils::remove_fs_lock(lock_file);
			return;
		}
		feeds.push_back(feed);
	}

	std::vector<std::string> tags = urlcfg->get_alltags();

	if (!do_export)
		std::cout << _("done.") << std::endl;

	if (do_export) {
		export_opml();
		utils::remove_fs_lock(lock_file);
		return;
	}

	// if the user wants to refresh on startup via configuration file, then do so,
	// but only if -r hasn't been supplied.
	if (!refresh_on_start && cfg->get_configvalue_as_bool("refresh-on-startup")) {
		refresh_on_start = true;
	}

	// hand over the important objects to the view
	v->set_config_container(cfg);
	v->set_keymap(&keys);
	// v->set_feedlist(feeds);
	v->set_tags(tags);

	// run the view
	v->run();

	std::cout << _("Cleaning up cache...");
	std::cout.flush();
	try {
		rsscache->cleanup_cache(feeds);
		std::cout << _("done.") << std::endl;
	} catch (const dbexception& e) {
		std::cout << _("failed: ") << e.what() << std::endl;
	}

	utils::remove_fs_lock(lock_file);
}

void controller::update_feedlist() {
	v->set_feedlist(feeds);
}

void controller::catchup_all() {
	try {
		rsscache->catchup_all();
	} catch (const dbexception& e) {
		char buf[1024];
		snprintf(buf, sizeof(buf), _("Error: couldn't mark all feeds read: %s"), e.what());
		v->show_error(buf);
		return;
	}
	for (std::vector<rss_feed>::iterator it=feeds.begin();it!=feeds.end();++it) {
		if (it->items().size() > 0) {
			for (std::vector<rss_item>::iterator jt=it->items().begin();jt!=it->items().end();++jt) {
				jt->set_unread_nowrite(false);
			}
		}
	}
}

void controller::mark_all_read(unsigned int pos) {
	if (pos < feeds.size()) {
		rss_feed& feed = feeds[pos];
		if (feed.rssurl().substr(0,6) == "query:") {
			rsscache->catchup_all(feed);
		} else {
			rsscache->catchup_all(feed.rssurl());
		}
		if (feed.items().size() > 0) {
			for (std::vector<rss_item>::iterator it=feed.items().begin();it!=feed.items().end();++it) {
				it->set_unread_nowrite_notify(false);
			}
		}
	}
}

void controller::reload(unsigned int pos, unsigned int max) {
	GetLogger().log(LOG_DEBUG, "controller::reload: pos = %u max = %u", pos, max);
	char msgbuf[1024];
	if (pos < feeds.size()) {
		rss_feed feed = feeds[pos];
		std::string msg;
		if (max > 0) {
			msg.append("(");
			std::ostringstream posstr;
			posstr << (pos+1);
			msg.append(posstr.str());
			msg.append("/");
			std::ostringstream maxstr;
			maxstr << max;
			msg.append(maxstr.str());
			msg.append(") ");
		}
		snprintf(msgbuf, sizeof(msgbuf), _("%sLoading %s..."), msg.c_str(), feed.rssurl().c_str());
		GetLogger().log(LOG_DEBUG, "controller::reload: before setting status");
		v->set_status(msgbuf);
		GetLogger().log(LOG_DEBUG, "controller::reload: after setting status");
				
		rss_parser parser(feed.rssurl().c_str(), rsscache, cfg, &ign);
		GetLogger().log(LOG_DEBUG, "controller::reload: created parser");
		try {
			if (parser.check_and_update_lastmodified()) {
				feed = parser.parse();
				GetLogger().log(LOG_DEBUG, "controller::reload: after parser.parse");
				if (!feed.is_empty()) {
					GetLogger().log(LOG_DEBUG, "controller::reload: feed is nonempty, saving");
					rsscache->externalize_rssfeed(feed);
					GetLogger().log(LOG_DEBUG, "controller::reload: after externalize_rssfeed");

					rsscache->internalize_rssfeed(feed);
					GetLogger().log(LOG_DEBUG, "controller::reload: after internalize_rssfeed");
					feed.set_tags(urlcfg->get_tags(feed.rssurl()));
					feeds[pos] = feed;
					v->notify_itemlist_change(feed);
				} else {
					GetLogger().log(LOG_DEBUG, "controller::reload: feed is empty, not saving");
				}

				for (std::vector<rss_item>::iterator it=feed.items().begin();it!=feed.items().end();++it) {
					if (cfg->get_configvalue_as_bool("podcast-auto-enqueue") && !it->enqueued() && it->enclosure_url().length() > 0) {
						GetLogger().log(LOG_DEBUG, "controller::reload: enclosure_url = `%s' enclosure_type = `%s'", it->enclosure_url().c_str(), it->enclosure_type().c_str());
						if (is_valid_podcast_type(it->enclosure_type())) {
							GetLogger().log(LOG_INFO, "controller::reload: enqueuing `%s'", it->enclosure_url().c_str());
							enqueue_url(it->enclosure_url());
							it->set_enqueued(true);
							rsscache->update_rssitem_unread_and_enqueued(*it, feed.rssurl());
						}
					}
				}

				
				v->set_feedlist(feeds);
				GetLogger().log(LOG_DEBUG, "controller::reload: after set_feedlist");
			}
			v->set_status("");
		} catch (const dbexception& e) {
			char buf[1024];
			snprintf(buf, sizeof(buf), _("Error while retrieving %s: %s"), feed.rssurl().c_str(), e.what());
			v->set_status(buf);
		} catch (const std::string& errmsg) {
			char buf[1024];
			snprintf(buf, sizeof(buf), _("Error while retrieving %s: %s"), feed.rssurl().c_str(), errmsg.c_str());
			v->set_status(buf);
		}
	} else {
		v->show_error(_("Error: invalid feed!"));
	}
}

rss_feed * controller::get_feed(unsigned int pos) {
	if (pos >= feeds.size()) {
		throw std::out_of_range(_("invalid feed index (bug)"));
	}
	return &(feeds[pos]);
}

void controller::reload_all() {
	GetLogger().log(LOG_DEBUG,"controller::reload_all: starting with reload all...");
	unsigned int unread_feeds, unread_articles;
	compute_unread_numbers(unread_feeds, unread_articles);
	time_t t1, t2, dt;
	t1 = time(NULL);
	for (unsigned int i=0;i<feeds.size();++i) {
		GetLogger().log(LOG_DEBUG, "controller::reload_all: reloading feed #%u", i);
		this->reload(i,feeds.size());
	}
	t2 = time(NULL);
	dt = t2 - t1;
	GetLogger().log(LOG_INFO, "controller::reload_all: reload took %d seconds", dt);

	unsigned int unread_feeds2, unread_articles2;
	compute_unread_numbers(unread_feeds2, unread_articles2);
	if (unread_feeds2 != unread_feeds || unread_articles2 != unread_articles) {
		char buf[2048];
		snprintf(buf,sizeof(buf),_("newsbeuter: finished reload, %u unread feeds (%u unread articles total)"), unread_feeds2, unread_articles2);
		this->notify(buf);
	}
}

void controller::notify(const std::string& msg) {
	if (cfg->get_configvalue_as_bool("notify-screen")) {
		GetLogger().log(LOG_DEBUG, "controller:notify: notifying screen");
		std::cout << "\033^" << msg << "\033\\";
		std::cout.flush();
	}
	if (cfg->get_configvalue_as_bool("notify-xterm")) {
		GetLogger().log(LOG_DEBUG, "controller:notify: notifying xterm");
		std::cout << "\033]2;" << msg << "\033\\";
		std::cout.flush();
	}
	if (cfg->get_configvalue("notify-program").length() > 0) {
		std::string prog = cfg->get_configvalue("notify-program");
		GetLogger().log(LOG_DEBUG, "controller:notify: notifying external program `%s'", prog.c_str());
		utils::run_command(prog, msg);
	}
	/* // TODO: implement Growl support
	if (cfg->get_configvalue_as_bool("notify-growl")) {
		std::vector<std::string> tokens = utils::tokenize(cfg->get_configvalue("growl-config"), ":");
		if (tokens.size() >= 1) {
			std::string hostname = tokens[0];
			std::string password;
			if (tokens.size() >= 2) {
				password = tokens[1];
			}
			growlnotifier->send_notify(hostname, password, "newsbeuter", msg);
		}
	}
	*/
}

void controller::compute_unread_numbers(unsigned int& unread_feeds, unsigned int& unread_articles) {
	unread_feeds = 0;
	unread_articles = 0;
	for (std::vector<rss_feed>::iterator it=feeds.begin();it!=feeds.end();++it) {
		unsigned int items = it->unread_item_count();
		if (items > 0) {
			++unread_feeds;
			unread_articles += items;
		}
	}
}

bool controller::trylock_reload_mutex() {
	if (reload_mutex->trylock()) {
		GetLogger().log(LOG_DEBUG, "controller::trylock_reload_mutex succeeded");
		return true;
	}
	GetLogger().log(LOG_DEBUG, "controller::trylock_reload_mutex failed");
	return false;
}

void controller::start_reload_all_thread() {
	GetLogger().log(LOG_INFO,"starting reload all thread");
	thread * dlt = new downloadthread(this);
	dlt->start();
}

void controller::version_information() {
	std::cout << PROGRAM_NAME << " " << PROGRAM_VERSION << " - " << PROGRAM_URL << std::endl;
	std::cout << "Copyright (C) 2006-2007 Andreas Krennmair" << std::endl << std::endl;

	struct utsname xuts;
	uname(&xuts);

	std::cout << "System: " << xuts.sysname << " " << xuts.release << " (" << xuts.machine << ")" << std::endl;
#if defined(__GNUC__) && defined(__VERSION__)
	std::cout << "Compiler: g++ " << __VERSION__ << std::endl;
#endif
	std::cout << "ncurses: " << curses_version() << " (compiled with " << NCURSES_VERSION << ")" << std::endl;
	std::cout << "libcurl: " << curl_version()  << " (compiled with " << LIBCURL_VERSION << ")" << std::endl;
	std::cout << "SQLite: " << sqlite3_libversion() << " (compiled with " << SQLITE_VERSION << ")" << std::endl;

	::exit(EXIT_SUCCESS);
}

void controller::usage(char * argv0) {
	char buf[2048];
	snprintf(buf, sizeof(buf), 
				_("%s %s\nusage: %s [-i <file>|-e] [-u <urlfile>] [-c <cachefile>] [-h]\n"
				"-e              export OPML feed to stdout\n"
				"-r              refresh feeds on start\n"
				"-i <file>       import OPML file\n"
				"-u <urlfile>    read RSS feed URLs from <urlfile>\n"
				"-c <cachefile>  use <cachefile> as cache file\n"
				"-C <configfile> read configuration from <configfile>\n"
				"-v              clean up cache thoroughly\n"
				"-o              activate offline mode (only applies to bloglines synchronization mode)\n"
				"-V              get version information\n"
				"-h              this help\n"), PROGRAM_NAME, PROGRAM_VERSION, argv0);
	std::cout << buf;
	::exit(EXIT_FAILURE);
}

void controller::import_opml(const char * filename) {
	nxml_t *data;
	nxml_data_t * root, * body;
	nxml_error_t ret;

	ret = nxml_new (&data);
	if (ret != NXML_OK) {
		puts (nxml_strerror (data, ret));
		return;
	}

	ret = nxml_parse_file (data, const_cast<char *>(filename));
	if (ret != NXML_OK) {
		puts (nxml_strerror (data, ret));
		return;
	}

	nxml_root_element (data, &root);

	if (root) {
		body = nxmle_find_element(data, root, "body", NULL);
		if (body) {
			GetLogger().log(LOG_DEBUG, "import_opml: found body");
			rec_find_rss_outlines(body, "");
			urlcfg->write_config();
		}
	}

	nxml_free(data);
	char buf[1024];
	snprintf(buf, sizeof(buf), _("Import of %s finished."), filename);
	std::cout << buf << std::endl;
}

void controller::export_opml() {
	std::cout << "<?xml version=\"1.0\"?>" << std::endl;
	std::cout << "<opml version=\"1.0\">" << std::endl;
	std::cout << "\t<head>" << std::endl << "\t\t<title>" PROGRAM_NAME " - Exported Feeds</title>" << std::endl << "\t</head>" << std::endl;
	std::cout << "\t<body>" << std::endl;
	for (std::vector<rss_feed>::iterator it=feeds.begin(); it != feeds.end(); ++it) {
		if (it->rssurl().substr(0,6) != "query:" && it->rssurl().substr(0,7) != "filter:") {
			std::string rssurl = utils::replace_all(it->rssurl(),"&","&amp;");
			std::string link = utils::replace_all(it->link(),"&", "&amp;");
			std::string title = utils::replace_all(it->title(),"&", "&amp;");

			rssurl = utils::replace_all(rssurl, "\"", "&quot;");
			link = utils::replace_all(link, "\"", "&quot;");
			title = utils::replace_all(title, "\"", "&quot;");
			std::cout << "\t\t<outline type=\"rss\" xmlUrl=\"" << rssurl << "\" htmlUrl=\"" << link << "\" title=\"" << title << "\" />" << std::endl;
		}
	}
	std::cout << "\t</body>" << std::endl;
	std::cout << "</opml>" << std::endl;
}

void controller::rec_find_rss_outlines(nxml_data_t * node, std::string tag) {
	while (node) {
		const char * url = nxmle_find_attribute(node, "xmlUrl", NULL);
		std::string newtag = tag;

		if (!url) {
			url = nxmle_find_attribute(node, "url", NULL);
		}

		if (node->type == NXML_TYPE_ELEMENT && strcmp(node->value,"outline")==0) {
			if (url) {

				GetLogger().log(LOG_DEBUG,"OPML import: found RSS outline with url = %s",url);

				bool found = false;

				GetLogger().log(LOG_DEBUG, "OPML import: size = %u", urlcfg->get_urls().size());
				if (urlcfg->get_urls().size() > 0) {
					for (std::vector<std::string>::iterator it = urlcfg->get_urls().begin(); it != urlcfg->get_urls().end(); ++it) {
						if (*it == url) {
							found = true;
						}
					}
				}

				if (!found) {
					GetLogger().log(LOG_DEBUG,"OPML import: added url = %s",url);
					urlcfg->get_urls().push_back(std::string(url));
					if (tag.length() > 0) {
						GetLogger().log(LOG_DEBUG, "OPML import: appending tag %s to url %s", tag.c_str(), url);
						urlcfg->get_tags(url).push_back(tag);
					}
				} else {
					GetLogger().log(LOG_DEBUG,"OPML import: url = %s is already in list",url);
				}
			} else {
				const char * text = nxmle_find_attribute(node, "text", NULL);
				if (!text)
					text = nxmle_find_attribute(node, "title", NULL);
				if (text) {
					if (newtag.length() > 0) {
						newtag.append("/");
					}
					newtag.append(text);
				}
			}
		}
		rec_find_rss_outlines(node->children, newtag);

		node = node->next;
	}
}



std::vector<rss_item> controller::search_for_items(const std::string& query, const std::string& feedurl) {
	std::vector<rss_item> items = rsscache->search_for_items(query, feedurl);
	GetLogger().log(LOG_DEBUG, "controller::search_for_items: setting feed pointers");
	for (std::vector<rss_item>::iterator it=items.begin();it!=items.end();++it) {
		it->set_feedptr(get_feed_by_url(it->feedurl()));
	}
	return items;
}

rss_feed * controller::get_feed_by_url(const std::string& feedurl) {
	for (std::vector<rss_feed>::iterator it=feeds.begin();it!=feeds.end();++it) {
		if (feedurl == it->rssurl())
			return &(*it);
	}
	return NULL; // shouldn't happen
}

bool controller::is_valid_podcast_type(const std::string& /* mimetype */) {
	return true;
	// return mimetype == "audio/mpeg" || mimetype == "video/x-m4v" || mimetype == "audio/x-mpeg";
}

void controller::enqueue_url(const std::string& url) {
	bool url_found = false;
	std::fstream f;
	f.open(queue_file.c_str(), std::fstream::in);
	if (f.is_open()) {
		do {
			std::string line;
			getline(f, line);
			if (!f.eof() && line.length() > 0) {
				if (line == url) {
					url_found = true;
					break;
				}
			}
		} while (!f.eof());
		f.close();
	}
	if (!url_found) {
		f.open(queue_file.c_str(), std::fstream::app | std::fstream::out);
		f << url << std::endl;
		f.close();
	}
}

void controller::reload_urls_file() {
	urlcfg->reload();
	std::vector<rss_feed> new_feeds;

	for (std::vector<std::string>::const_iterator it=urlcfg->get_urls().begin();it!=urlcfg->get_urls().end();++it) {
		bool found = false;
		for (std::vector<rss_feed>::iterator jt=feeds.begin();jt!=feeds.end();++jt) {
			if (*it == jt->rssurl()) {
				found = true;
				jt->set_tags(urlcfg->get_tags(*it));
				new_feeds.push_back(*jt);
				break;
			}
		}
		if (!found) {
			rss_feed new_feed(rsscache);
			new_feed.set_rssurl(*it);
			new_feed.set_tags(urlcfg->get_tags(*it));
			try {
				rsscache->internalize_rssfeed(new_feed);
			} catch(const dbexception& e) {
				throw e; // TODO ?
			}
			new_feeds.push_back(new_feed);
		}
	}

	feeds = new_feeds;

	update_feedlist();
}

void controller::set_feedptrs(rss_feed& feed) {
	for (std::vector<rss_item>::iterator it=feed.items().begin();it!=feed.items().end();++it) {
		it->set_feedptr(&feed);
	}
}

std::string controller::bookmark(const std::string& url, const std::string& title, const std::string& description) {
	std::string bookmark_cmd = cfg->get_configvalue("bookmark-cmd");
	if (bookmark_cmd.length() > 0) {
		char * my_argv[4];
		my_argv[0] = const_cast<char *>("/bin/sh");
		my_argv[1] = const_cast<char *>("-c");

		// wow. what an abuse.
		std::string cmdline = bookmark_cmd + " " + stfl::quote(url) + " " + stfl::quote(title) + " " + stfl::quote(description);

		my_argv[2] = const_cast<char *>(cmdline.c_str());
		my_argv[3] = NULL;

		return utils::run_program(my_argv, "");
	} else {
		return _("bookmarking support is not configured. Please set the configuration variable `bookmark-cmd' accordingly.");
	}
}


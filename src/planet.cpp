#include <planet.h>
#include <logger.h>
#include <stflpp.h>
#include <config.h>
#include <urlreader.h>
#include <exceptions.h>
#include <utils.h>

#include <string>
#include <iostream>
#include <sstream>

#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include <signal.h>
#include <fcntl.h>

using namespace newsbeuter;

namespace nbplanet {

static std::string lock_file = "lock.pid";

void ctrl_c_action(int sig) {
	GetLogger().log(LOG_DEBUG,"caugh signal %d",sig);
	::unlink(lock_file.c_str());
	if (SIGSEGV == sig) {
		fprintf(stderr,"%s\n", _("Segmentation fault."));
	}
	::exit(EXIT_FAILURE);
}


planet::planet() : rsscache(0), cfg(0), config_file("config"), url_file("urls"), cache_file("cache.db"), verbose(0) {
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

}

planet::~planet() {
	delete rsscache;
	delete cfg;
}

void planet::run(int argc, char * argv[]) {
	int c;
	char msgbuf[1024];

	std::string output_file;

	::signal(SIGINT, ctrl_c_action);
	::signal(SIGSEGV, ctrl_c_action);

	do {
		if((c = ::getopt(argc,argv,"hu:c:C:d:l:v"))<0)
			continue;
		switch (c) {
			case ':': /* fall-through */
			case '?': /* missing option */
				usage(argv[0]);
				break;
			case 'h':
				usage(argv[0]);
				break;
			case 'u':
				url_file = optarg;
				break;
			case 'c':
				cache_file = optarg;
				break;
			case 'v':
				++verbose;
				break;
			case 'C':
				config_file = optarg;
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

	urlcfg.load_config(url_file);

	if (urlcfg.get_urls().size() == 0) {
		GetLogger().log(LOG_ERROR,"no URLs configured.");
		snprintf(msgbuf, sizeof(msgbuf), _("Error: no URLs configured. Please fill the file %s with RSS feed URLs or import an OPML file."), url_file.c_str());
		std::cout << msgbuf << std::endl << std::endl;
		usage(argv[0]);
	}

	pid_t pid;
	if (!utils::try_fs_lock(lock_file, pid)) {
		GetLogger().log(LOG_ERROR,"an instance is alredy running: pid = %u",pid);
		snprintf(msgbuf, sizeof(msgbuf), _("Error: an instance of %s is already running (PID: %u)"), "podbeuter", pid);
		std::cout << msgbuf << std::endl;
		return;
	}

	configparser cfgparser(config_file.c_str());
	cfg = new configcontainer();
	cfg->register_commands(cfgparser);

	try {
		cfgparser.parse();
	} catch (const configexception& ex) {
		GetLogger().log(LOG_ERROR,"an exception occured while parsing the configuration file: %s",ex.what());
		std::cout << ex.what() << std::endl;
		utils::remove_fs_lock(lock_file);
		return;	
	}

	output_file = cfg->get_configvalue("planet-output");
	if (output_file.length() == 0) {
		snprintf(msgbuf, sizeof(msgbuf), _("Error: your configuration is incomplete. Please set \"planet-output\" correctly to be able to generate output."));
		std::cout << msgbuf << std::endl;
		utils::remove_fs_lock(lock_file);
		return;
	}

	template_file = cfg->get_configvalue("planet-template");
	if (template_file.length() == 0) {
		snprintf(msgbuf, sizeof(msgbuf), _("Error: your configuration is incomplete. Please set \"planet-template\" correctly to be able to generate output."));
		std::cout << msgbuf << std::endl;
		utils::remove_fs_lock(lock_file);
		return;
	}


	if (output_file.substr(0,2) == "~/") {
		std::string suffix = output_file.substr(2,output_file.length()-2);
		char * homedir;
		if (!(homedir = ::getenv("HOME"))) {
			struct passwd * spw = ::getpwuid(::getuid());
			if (spw) {
				homedir = spw->pw_dir;
			} else {
				std::cout << _("Fatal error: couldn't determine home directory!") << std::endl;
				char buf[1024];
				snprintf(buf, sizeof(buf), _("Please set the HOME environment variable or add a valid user for UID %u!"), ::getuid());
				std::cout << buf << std::endl;
				utils::remove_fs_lock(lock_file);
				::exit(EXIT_FAILURE);
			}
		}
		output_file = homedir;
		output_file.append(NEWSBEUTER_PATH_SEP);
		output_file.append(suffix);
	}

	rsscache = new cache(cache_file,cfg);

	for (std::vector<std::string>::const_iterator it=urlcfg.get_urls().begin(); it != urlcfg.get_urls().end(); ++it) {
		rss_feed feed(rsscache);
		feed.set_rssurl(*it);
		feed.set_tags(urlcfg.get_tags(*it));
		rsscache->internalize_rssfeed(feed);
		feeds.push_back(feed);
	}

	reload_all();
	
	std::vector<rss_item> rss_items;
	unsigned int limit = cfg->get_configvalue_as_int("planet-limit");
	rsscache->get_latest_items(rss_items, limit);

	utils::planet_generate_html(feeds, rss_items, template_file, output_file);

	rsscache->cleanup_cache(feeds);

	utils::remove_fs_lock(lock_file);
}

void planet::reload_all() {
	for (unsigned int i=0;i<feeds.size();++i) {
		reload(i);
	}
}

void planet::reload(unsigned int pos) {
	rss_feed& feed = feeds[pos];
	char msg[1024];
	if (verbose>=1) {
		snprintf(msg, sizeof(msg), "Reloading %s...", feed.rssurl().c_str());
		std::cout << msg << std::endl;
	}
	rss_parser parser(feed.rssurl().c_str(), rsscache, cfg);
	try {
		feed = parser.parse();
		
		rsscache->externalize_rssfeed(feed);

		rsscache->internalize_rssfeed(feed);
		feed.set_tags(urlcfg.get_tags(feed.rssurl()));
		feeds[pos] = feed;

	} catch (const std::string& errmsg) {
		snprintf(msg, sizeof(msg), _("Error while retrieving %s: %s"), feed.rssurl().c_str(), errmsg.c_str());
		std::cout << msg << std::endl;
	}
}


void planet::usage(char * argv0) {
	char buf[2048];
	snprintf(buf, sizeof(buf), 
				_("%s %s\nusage: %s [-u <urlfile>] [-c <cachefile>] [-C <configfile>] [-v] [-h]\n"
				"-u <urlfile>    read RSS feed URLs from <urlfile>\n"
				"-c <cachefile>  use <cachefile> as cache file\n"
				"-C <configfile> read configuration from <configfile>\n"
				"-v              make program output more verbose\n"
				"-h              this help\n"), "nb-planet", PROGRAM_VERSION, argv0);
	std::cout << buf;
	::exit(EXIT_FAILURE);
}

}

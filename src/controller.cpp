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
#include <sstream>
#include <cstdlib>
#include <iostream>
#include <fstream>

#include <sys/time.h>
#include <ctime>
#include <signal.h>

#include <nxml.h>

#include <sys/types.h>
#include <pwd.h>

#include <config.h>

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
	::signal(SIGSEGV, ctrl_c_action);
#endif

	bool do_import = false, do_export = false;
	std::string importfile;

	do {
		if((c = ::getopt(argc,argv,"i:erhu:c:C:d:l:"))<0)
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

	if (do_import) {
		GetLogger().log(LOG_INFO,"Importing OPML file from %s",importfile.c_str());
		import_opml(importfile.c_str());
		return;
	}

	if (urlcfg.get_urls().size() == 0) {
		GetLogger().log(LOG_ERROR,"no URLs configured.");
		snprintf(msgbuf, sizeof(msgbuf), _("Error: no URLs configured. Please fill the file %s with RSS feed URLs or import an OPML file."), url_file.c_str());
		std::cout << msgbuf << std::endl << std::endl;
		usage(argv[0]);
	}

	if (!do_export) {
		snprintf(msgbuf, sizeof(msgbuf), _("Starting %s %s..."), PROGRAM_NAME, PROGRAM_VERSION);
		std::cout << msgbuf << std::endl;

		pid_t pid;
		if (!utils::try_fs_lock(lock_file, pid)) {
			GetLogger().log(LOG_ERROR,"an instance is alredy running: pid = %u",pid);
			snprintf(msgbuf, sizeof(msgbuf), _("Error: an instance of %s is already running (PID: %u)"), PROGRAM_NAME, pid);
			std::cout << msgbuf << std::endl;
			return;
		}
	}
	
	if (!do_export)
		std::cout << _("Loading configuration...");
	std::cout.flush();
	
	configparser cfgparser(config_file.c_str());
	cfg = new configcontainer();
	cfg->register_commands(cfgparser);
	colormanager * colorman = new colormanager();
	colorman->register_commands(cfgparser);

	keymap keys;
	cfgparser.register_handler("bind-key",&keys);
	cfgparser.register_handler("unbind-key",&keys);

	try {
		cfgparser.parse();
	} catch (const configexception& ex) {
		GetLogger().log(LOG_ERROR,"an exception occured while parsing the configuration file: %s",ex.what());
		std::cout << ex.what() << std::endl;
		utils::remove_fs_lock(lock_file);
		return;	
	}

	if (colorman->colors_loaded())
		colorman->set_colors(v);
	delete colorman;
	
	if (!do_export)
		std::cout << _("done.") << std::endl;

	if (!do_export)
		std::cout << _("Loading articles from cache...");
	std::cout.flush();

	rsscache = new cache(cache_file,cfg);

	for (std::vector<std::string>::const_iterator it=urlcfg.get_urls().begin(); it != urlcfg.get_urls().end(); ++it) {
		rss_feed feed(rsscache);
		feed.set_rssurl(*it);
		feed.set_tags(urlcfg.get_tags(*it));
		rsscache->internalize_rssfeed(feed);
		feeds.push_back(feed);
	}

	std::vector<std::string> tags = urlcfg.get_alltags();

	if (!do_export)
		std::cout << _("done.") << std::endl;

	if (do_export) {
		export_opml();
		utils::remove_fs_lock(lock_file);
		return;
	}

	v->set_config_container(cfg);
	v->set_keymap(&keys);
	v->set_feedlist(feeds);
	// v->run_feedlist(tags);
	v->set_tags(tags);
	v->run();

	std::cout << _("Cleaning up cache...");
	std::cout.flush();
	rsscache->cleanup_cache(feeds);
	std::cout << _("done.") << std::endl;

	utils::remove_fs_lock(lock_file);
}

void controller::update_feedlist() {
	v->set_feedlist(feeds);
}

void controller::catchup_all() {
	rsscache->catchup_all();
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
		rsscache->catchup_all(feed.rssurl());
		if (feed.items().size() > 0) {
			for (std::vector<rss_item>::iterator it=feed.items().begin();it!=feed.items().end();++it) {
				it->set_unread_nowrite(false);
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
				
		rss_parser parser(feed.rssurl().c_str(), rsscache, cfg);
		GetLogger().log(LOG_DEBUG, "controller::reload: created parser");
		try {
			feed = parser.parse();
			GetLogger().log(LOG_DEBUG, "controller::reload: after parser.parse");
			
			rsscache->externalize_rssfeed(feed);
			GetLogger().log(LOG_DEBUG, "controller::reload: after externalize_rssfeed");

			rsscache->internalize_rssfeed(feed);
			GetLogger().log(LOG_DEBUG, "controller::reload: after internalize_rssfeed");
			feed.set_tags(urlcfg.get_tags(feed.rssurl()));
			feeds[pos] = feed;

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
			v->set_status("");
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
	for (unsigned int i=0;i<feeds.size();++i) {
		GetLogger().log(LOG_DEBUG, "controller::reload_all: reloading feed #%u", i);
		this->reload(i,feeds.size());
	}
}

void controller::start_reload_all_thread() {
	if (reload_mutex->trylock()) {
		GetLogger().log(LOG_INFO,"starting reload all thread");
		thread * dlt = new downloadthread(this);
		dlt->start();
	} else {
		GetLogger().log(LOG_INFO,"reload mutex is currently locked");
	}
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
		puts (nxml_strerror (ret));
		return;
	}

	ret = nxml_parse_file (data, const_cast<char *>(filename));
	if (ret != NXML_OK) {
		puts (nxml_strerror (ret));
		return;
	}

	nxml_root_element (data, &root);

	if (root) {
		body = nxmle_find_element(data, root, "body", NULL);
		if (body) {
			rec_find_rss_outlines(body, "");
			urlcfg.write_config();
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
		std::cout << "\t\t<outline type=\"rss\" xmlUrl=\"" << it->rssurl() << "\" title=\"" << it->title() << "\" />" << std::endl;
	}
	std::cout << "\t</body>" << std::endl;
	std::cout << "</opml>" << std::endl;
}

void controller::rec_find_rss_outlines(nxml_data_t * node, std::string tag) {
	while (node) {
		char * url = nxmle_find_attribute(node, "xmlUrl", NULL);
		char * type = nxmle_find_attribute(node, "type", NULL);
		std::string newtag = tag;

		if (!url) {
			url = nxmle_find_attribute(node, "url", NULL);
		}

		if (node->type == NXML_TYPE_ELEMENT && strcmp(node->value,"outline")==0) {
			if (type && (strcmp(type,"rss")==0 || strcmp(type,"link")==0)) {
				if (url) {

					GetLogger().log(LOG_DEBUG,"OPML import: found RSS outline with url = %s",url);

					bool found = false;

					for (std::vector<std::string>::iterator it = urlcfg.get_urls().begin(); it != urlcfg.get_urls().end(); ++it) {
						if (*it == url) {
							found = true;
						}
					}

					if (!found) {
						GetLogger().log(LOG_DEBUG,"OPML import: added url = %s",url);
						urlcfg.get_urls().push_back(std::string(url));
						if (tag.length() > 0) {
							GetLogger().log(LOG_DEBUG, "OPML import: appending tag %s to url %s", tag.c_str(), url);
							urlcfg.get_tags(url).push_back(tag);
						}
					} else {
						GetLogger().log(LOG_DEBUG,"OPML import: url = %s is already in list",url);
					}
				}
			} else {
				char * text = nxmle_find_attribute(node, "text", NULL);
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
	return rsscache->search_for_items(query, feedurl);
}

rss_feed * controller::get_feed_by_url(const std::string& feedurl) {
	for (std::vector<rss_feed>::iterator it=feeds.begin();it!=feeds.end();++it) {
		if (feedurl == it->rssurl())
			return &(*it);
	}
	return NULL; // shouldn't happen
}

bool controller::is_valid_podcast_type(const std::string& mimetype) {
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


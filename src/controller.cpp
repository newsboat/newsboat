#include <view.h>
#include <controller.h>
#include <configparser.h>
#include <configcontainer.h>
#include <exceptions.h>
#include <downloadthread.h>
#include <sstream>
#include <cstdlib>
#include <iostream>

#include <sys/time.h>
#include <ctime>

#include <nxml.h>

#include <sys/types.h>
#include <pwd.h>

#include <config.h>

using namespace noos;

controller::controller() : v(0), rsscache(0), url_file("urls"), cache_file("cache.db"), config_file("config") {
	std::ostringstream cfgfile;

	if (getenv("HOME")) {
		config_dir = ::getenv("HOME");
	} else {
		struct passwd * spw = ::getpwuid(::getuid());
		if (spw) {
			config_dir = spw->pw_dir;
		} else {
			std::cout << "Fatal error: couldn't determine home directory!" << std::endl;
			std::cout << "Please set the HOME environment variable or add a valid user for UID " << ::getuid() << "!" << std::endl;
			::exit(EXIT_FAILURE);
		}
	}

	config_dir.append(NOOS_PATH_SEP);
	config_dir.append(NOOS_CONFIG_SUBDIR);
	mkdir(config_dir.c_str(),0700); // create configuration directory if it doesn't exist

	url_file = config_dir + std::string(NOOS_PATH_SEP) + url_file;
	cache_file = config_dir + std::string(NOOS_PATH_SEP) + cache_file;
	config_file = config_dir + std::string(NOOS_PATH_SEP) + config_file;
	reload_mutex = new mutex();
}

controller::~controller() {
	if (rsscache)
		delete rsscache;
	delete reload_mutex;
}

void controller::set_view(view * vv) {
	v = vv;
}

void controller::run(int argc, char * argv[]) {
	int c;

	bool do_import = false, do_export = false;
	std::string importfile;

	do {
		c = ::getopt(argc,argv,"i:ehu:c:C:");
		if (c < 0)
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
			default:
				std::cout << argv[0] << ": unknown option: -" << static_cast<char>(c) << std::endl;
				usage(argv[0]);
				break;
		}
	} while (c != -1);

	urlcfg.load_config(url_file);

	if (do_import) {
		import_opml(importfile.c_str());
		return;
	}

	if (urlcfg.get_urls().size() == 0) {
		std::cout << "Error: no URLs configured. Please fill the file " << url_file << " with RSS feed URLs or import an OPML file." << std::endl << std::endl;
		usage(argv[0]);
	}

	if (!do_export) {
		std::cout << "Starting " << PROGRAM_NAME << " " << PROGRAM_VERSION << "..." << std::endl << std::endl;
	}
	
	if (!do_export)
		std::cout << "Loading configuration...";
	std::cout.flush();
	
	configparser cfgparser(config_file.c_str());
	configcontainer cfg;
	keymap keys;
	cfg.register_commands(cfgparser);
	cfgparser.register_handler("bind-key",&keys);
	cfgparser.register_handler("unbind-key",&keys);

	try {
		cfgparser.parse();
	} catch (const configexception& ex) {
		std::cout << ex.what() << std::endl;
		return;	
	}
	
	if (!do_export)
		std::cout << "done." << std::endl;

	if (!do_export)
		std::cout << "Loading articles from cache...";
	std::cout.flush();

	rsscache = new cache(cache_file,&cfg);

	for (std::vector<std::string>::const_iterator it=urlcfg.get_urls().begin(); it != urlcfg.get_urls().end(); ++it) {
		rss_feed feed(rsscache);
		feed.set_rssurl(*it);
		rsscache->internalize_rssfeed(feed);
		feeds.push_back(feed);
	}
	// rsscache->cleanup_cache(feeds);
	if (!do_export)
		std::cout << "done." << std::endl;

	if (do_export) {
		export_opml();
		return;
	}

	v->set_config_container(&cfg);
	v->set_keymap(&keys);
	v->set_feedlist(feeds);
	v->run_feedlist();

	std::cout << "Cleaning up cache...";
	std::cout.flush();
	rsscache->cleanup_cache(feeds);
	/*
	for (std::vector<rss_feed>::iterator it=feeds.begin(); it != feeds.end(); ++it) {
		// rsscache->externalize_rssfeed(*it);
	}
	*/
	std::cout << "done." << std::endl;
}

void controller::update_feedlist() {
	v->set_feedlist(feeds);
}

bool controller::open_item(rss_item& item) {
	bool show_next_unread = v->run_itemview(item);
	item.set_unread(false); // XXX: see TODO list
	return show_next_unread;
}

void controller::catchup_all() {
	for (unsigned int i=0;i<feeds.size();++i) {
		mark_all_read(i);
	}
}

void controller::mark_all_read(unsigned int pos) {
	if (pos < feeds.size()) {
		rss_feed& feed = feeds[pos];
		for (std::vector<rss_item>::iterator it = feed.items().begin(); it != feed.items().end(); ++it) {
			it->set_unread(false);
		}
	}
}



void controller::open_feed(unsigned int pos) {
	if (pos < feeds.size()) {
		v->set_status("Opening feed...");

		rss_feed& feed = feeds[pos];

		v->set_status("");

		if (feed.items().size() == 0) {
			v->show_error("Error: feed contains no items!");
		} else {
			v->run_itemlist(feed);
			// rsscache->externalize_rssfeed(feed); // save possibly changed unread flags
			v->set_feedlist(feeds);
		}
	} else {
		v->show_error("Error: invalid feed!");
	}
}

void controller::reload(unsigned int pos, unsigned int max) {
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
		msg.append("Loading ");
		msg.append(feed.rssurl());
		msg.append("...");
		v->set_status(msg.c_str());
				
		rss_parser parser(feed.rssurl().c_str(), rsscache);
		feed = parser.parse();
		
		/*
		struct timeval tv1, tv2;
		gettimeofday(&tv1, NULL);
		*/
		
		rsscache->externalize_rssfeed(feed);
		
		/*
		gettimeofday(&tv2, NULL);
		
		unsigned long long t1 = tv1.tv_sec*1000000 + tv1.tv_usec;
		unsigned long long t2 = tv2.tv_sec*1000000 + tv2.tv_usec;
		
		std::cerr << "time for externalizing: " << (t2-t1)/1000 << " ms" << std::endl;
		*/
		
		rsscache->internalize_rssfeed(feed);
		feeds[pos] = feed;
		
		v->set_status("");
		v->set_feedlist(feeds);
	} else {
		v->show_error("Error: invalid feed!");
	}
}

void controller::reload_all() {
	for (unsigned int i=0;i<feeds.size();++i) {
		this->reload(i,feeds.size());
	}
}

void controller::start_reload_all_thread() {
	if (reload_mutex->trylock()) {
		thread * dlt = new downloadthread(this);
		dlt->start();
	}
}

void controller::usage(char * argv0) {
	std::cout << PROGRAM_NAME << " " << PROGRAM_VERSION << std::endl;
	std::cout << "usage: " << argv0 << " [-i <file>|-e] [-u <urlfile>] [-c <cachefile>] [-h]" << std::endl;
	std::cout << "-e              export OPML feed to stdout" << std::endl;
	std::cout << "-i <file>       import OPML file" << std::endl;
	std::cout << "-u <urlfile>    read RSS feed URLs from <urlfile>" << std::endl;
	std::cout << "-c <cachefile>  use <cachefile> as cache file" << std::endl;
	std::cout << "-C <configfile> read configuration from <configfile>" << std::endl;
	std::cout << "-h              this help" << std::endl;
	::exit(EXIT_FAILURE);
}

void controller::import_opml(const char * filename) {
	nxml_t *data;
	nxml_data_t * root, * body;
	nxml_error_t ret;

	ret = nxml_new (&data);
	if (ret != NXML_OK) {
		puts (nxml_strerror (ret)); // TODO
		return;
	}

	ret = nxml_parse_file (data, const_cast<char *>(filename));
	if (ret != NXML_OK) {
		puts (nxml_strerror (ret)); // TODO
		return;
	}

	nxml_root_element (data, &root);

	if (root) {
		body = nxmle_find_element(data, root, "body", NULL);
		if (body) {
			rec_find_rss_outlines(body);
			urlcfg.write_config();
		}
	}

	nxml_free(data);
	std::cout << "Import of " << filename << " finished." << std::endl;
}

void controller::export_opml() {
	std::cout << "<?xml version=\"1.0\"?>" << std::endl;
	std::cout << "<opml version=\"1.0\">" << std::endl;
	std::cout << "\t<head>" << std::endl << "\t\t<title>noos - Exported Feeds</title>" << std::endl << "\t</head>" << std::endl;
	std::cout << "\t<body>" << std::endl;
	for (std::vector<rss_feed>::iterator it=feeds.begin(); it != feeds.end(); ++it) {
		std::cout << "\t\t<outline type=\"rss\" xmlUrl=\"" << it->rssurl() << "\" title=\"" << it->title() << "\" />" << std::endl;
	}
	std::cout << "\t</body>" << std::endl;
	std::cout << "</opml>" << std::endl;
}

void controller::rec_find_rss_outlines(nxml_data_t * node) {
	while (node) {
		char * url = nxmle_find_attribute(node, "xmlUrl", NULL);
		char * type = nxmle_find_attribute(node, "type", NULL);

		if (node->type == NXML_TYPE_ELEMENT && strcmp(node->value,"outline")==0 && type && strcmp(type,"rss")==0 && url) {

			bool found = false;

			for (std::vector<std::string>::iterator it = urlcfg.get_urls().begin(); it != urlcfg.get_urls().end(); ++it) {
				if (*it == url) {
					found = true;
				}
			}

			if (!found) {
				urlcfg.get_urls().push_back(std::string(url));
			}

		}

		rec_find_rss_outlines(node->children);

		node = node->next;
	}
}

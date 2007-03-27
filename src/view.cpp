#include <feedlist.h>
#include <itemlist.h>
#include <itemview.h>
#include <help.h>
#include <filebrowser.h>
#include <urlview.h>
#include <selecttag.h>
#include <search.h>
#include <formaction.h>
#include <feedlist_formaction.h>
#include <itemlist_formaction.h>

#include <logger.h>
#include <reloadthread.h>
#include <exception.h>

#include <iostream>
#include <iomanip>
#include <fstream>

#include <assert.h>
#include <libgen.h>
#include <sys/types.h>
#include <dirent.h>
#include <grp.h>
#include <pwd.h>
#include <unistd.h>
#include <config.h>
#include <sys/param.h>
#include <string.h>


extern "C" {
#include <stfl.h>
}

#include <view.h>
#include <rss.h>
#include <htmlrenderer.h>
#include <cstring>
#include <cstdio>
#include <iostream>
#include <sstream>

using namespace newsbeuter;

view::view(controller * c) : ctrl(c), cfg(0), keys(0), mtx(0) /*,
		feedlist_form(feedlist_str), itemlist_form(itemlist_str), itemview_form(itemview_str), 
		help_form(help_str), filebrowser_form(filebrowser_str), urlview_form(urlview_str), 
		selecttag_form(selecttag_str), search_form(search_str) */ { 
	mtx = new mutex();

	feedlist = new feedlist_formaction(this, feedlist_str);
	itemlist = new itemlist_formaction(this, itemlist_str);
	// TODO: create all formaction objects

	// push the dialog to start with onto the stack
	formaction_stack.push_front(feedlist);
}

view::~view() {
	stfl::reset();
	delete mtx;
	delete feedlist;
	delete itemlist;
}

void view::set_config_container(configcontainer * cfgcontainer) {
	cfg = cfgcontainer;	
}

void view::set_keymap(keymap * k) {
	keys = k;
}

void view::set_status(const char * msg) {
	mtx->lock();
	if (formaction_stack.size() > 0) {
		stfl::form& form = (*formaction_stack.begin())->get_form();
		form.set("msg",msg);
		form.run(-1);
	}
	mtx->unlock();
}

void view::show_error(const char * msg) {
	set_status(msg);
}

void view::run() {

	feedlist->init();
	itemlist->init();

	while (formaction_stack.size() > 0) {
		formaction * fa = *(formaction_stack.begin());

		fa->prepare();

		const char * event = fa->get_form().run(0);
		if (!event) continue;

		operation op = keys->get_operation(event);

		fa->process_operation(op);
	}

	stfl::reset();
}

#if 0
void view::run_search(const std::string& feedurl) {
	bool quit = false;
	bool rebuild_list = false;
	bool set_listfocus = false;

	std::vector<rss_item> items;

	view_stack.push_front(&search_form);

	set_search_keymap_hint();

	search_form.set("msg","");

	search_form.set("head",_("Search Articles"));

	search_form.set("searchprompt",_("Search for: "));

	search_form.modify("results","replace_inner","{list}");

	search_form.set_focus("query");

	do {

		if (rebuild_list) {
			std::string code = "{list";

			unsigned int i=0;
			for (std::vector<rss_item>::iterator it = items.begin(); it != items.end(); ++it, ++i) {
				std::string line = "{listitem[";
				std::ostringstream x;
				x << i;
				line.append(x.str());
				line.append("] text:");
				std::string title;
				char buf[20];
				snprintf(buf,sizeof(buf),"%4u ",i+1);
				title.append(buf);
				if (it->unread()) {
					title.append("N ");
				} else {
					title.append("  ");
				}
				char datebuf[64];
				time_t t = it->pubDate_timestamp();
				struct tm * stm = localtime(&t);
				strftime(datebuf,sizeof(datebuf), "%b %d   ", stm);
				title.append(datebuf);
				title.append(it->title());
				line.append(stfl::quote(title));
				line.append("}");
				code.append(line);
			}

			code.append("}");

			search_form.modify("results","replace_inner",code);

			if (set_listfocus) {
				search_form.run(-1);
				search_form.set_focus("results");
				GetLogger().log(LOG_DEBUG, "view::run_search: setting focus to results");
				set_listfocus = false;
			}

			rebuild_list = false;
		}

		operation op;
		const char * event = search_form.run(0);
		if (!event) continue;

		op = keys->get_operation(event);

		GetLogger().log(LOG_DEBUG, "view::run_search: event = %s operation = %d", event, op);

		switch (op) {
			case OP_OPEN: {
					std::string querytext = search_form.get("querytext");
					std::string focus = search_form.get_focus();
					if (focus == "query") {
						if (querytext.length() > 0) {
							items = ctrl->search_for_items(querytext, feedurl);
							if (items.size() > 0) {
								char buf[1024];
								search_form.set("listpos", "0");
								snprintf(buf, sizeof(buf), _("Search Articles - %u results"), items.size());
								search_form.set("head", buf);
								set_listfocus = true;
								rebuild_list = true;
							} else {
								show_error(_("No results."));
							}
						} else {
							quit = true;
						}
					} else {
						std::string itemposname = search_form.get("listpos");
						GetLogger().log(LOG_INFO, "view::run_search: opening item at pos `%s'", itemposname.c_str());
						if (itemposname.length() > 0) {
							std::istringstream posname(itemposname);
							unsigned int pos = 0;
							posname >> pos;
							rss_feed tmpfeed = ctrl->get_feed_by_url(items[pos].feedurl());
							tmpfeed.items().push_back(items[pos]);
							ctrl->open_item(tmpfeed, items[pos].guid());
							rebuild_list = true;
						} else {
							show_error(_("No item selected!")); // should not happen
						}
					}
				}
				break;
			case OP_SEARCH:
				search_form.set_focus("query");
				break;
			case OP_QUIT:
				quit = true;
				break;
			case OP_HELP:
				run_help();
				set_status("");
				break;
			default:
				break;
		}
	} while (!quit);

	view_stack.pop_front();
}
#endif

std::string view::get_filename_suggestion(const std::string& s) {
	std::string retval;
	for (unsigned int i=0;i<s.length();++i) {
		if (isalnum(s[i]))
			retval.append(1,s[i]);
		else if (s[i] == '/' || s[i] == ' ' || s[i] == '\r' || s[i] == '\n') 
			retval.append(1,'_');
	}
	if (retval.length() == 0)
		retval = "article.txt";
	else
		retval.append(".txt");
	GetLogger().log(LOG_DEBUG,"view::get_filename_suggestion: %s -> %s", s.c_str(), retval.c_str());
	return retval;	
}

void view::write_item(const rss_item& item, const std::string& filename) {
	std::vector<std::string> lines;
	std::vector<linkpair> links; // not used
	
	std::string title(_("Title: "));
	title.append(item.title());
	lines.push_back(title);
	
	std::string author(_("Author: "));
	author.append(item.author());
	lines.push_back(author);
	
	std::string date(_("Date: "));
	date.append(item.pubDate());
	lines.push_back(date);
	
	lines.push_back(std::string(""));
	
	htmlrenderer rnd(80);
	rnd.render(item.description(), lines, links, item.feedurl());

	std::fstream f;
	f.open(filename.c_str(),std::fstream::out);
	if (!f.is_open())
		throw exception(errno);
		
	for (std::vector<std::string>::iterator it=lines.begin();it!=lines.end();++it) {
		f << *it << std::endl;	
	}
}

std::string view::get_rwx(unsigned short val) {
	std::string str;
	for (int i=0;i<3;++i) {
		unsigned char bits = val % 8;
		val /= 8;
		switch (bits) {
			case 0:
				str = std::string("---") + str;
				break;
			case 1:
				str = std::string("--x") + str;
				break;
			case 2:
				str = std::string("-w-") + str;
				break;
			case 3:
				str = std::string("-wx") + str;
				break;
			case 4:
				str = std::string("r--") + str;
				break;
			case 5:
				str = std::string("r-x") + str;
				break;
			case 6:
				str = std::string("rw-") + str;
				break;
			case 7:
				str = std::string("rwx") + str;
		}	
	}
	return str;
}

std::string view::fancy_quote(const std::string& s) {
	std::string x;
	for (unsigned int i=0;i<s.length();++i) {
		if (s[i] != ' ') {
			x.append(1,s[i]);
		} else {
			x.append(1,'/');
		}	
	}	
	return x;
}

std::string view::fancy_unquote(const std::string& s) {
	std::string x;
	for (unsigned int i=0;i<s.length();++i) {
		if (s[i] != '/') {
			x.append(1,s[i]);
		} else {
			x.append(1,' ');
		}	
	}	
	return x;	
}

std::string view::add_file(std::string filename) {
	std::string retval;
	struct stat sb;
	if (::stat(filename.c_str(),&sb)==0) {
		char type = '?';
		if (sb.st_mode & S_IFREG)
			type = '-';
		else if (sb.st_mode & S_IFDIR)
			type = 'd';
		else if (sb.st_mode & S_IFBLK)
			type = 'b';
		else if (sb.st_mode & S_IFCHR)
			type = 'c';
		else if (sb.st_mode & S_IFIFO)
			type = 'p';
		else if (sb.st_mode & S_IFLNK)
			type = 'l';
			
		std::string rwxbits = get_rwx(sb.st_mode & 0777);
		std::string owner = "????????", group = "????????";
		
		struct passwd * spw = getpwuid(sb.st_uid);
		if (spw) {
			owner = spw->pw_name;
			for (int i=owner.length();i<8;++i) {
				owner.append(" ");	
			}	
		}
		struct group * sgr = getgrgid(sb.st_gid);
		if (sgr) {
			group = sgr->gr_name;
			for (int i=group.length();i<8;++i) {
				group.append(" ");
			}
		}
		
		std::ostringstream os;
		os << std::setw(12) << sb.st_size;
		std::string sizestr = os.str();
		
		std::string line;
		line.append(1,type);
		line.append(rwxbits);
		line.append(" ");
		line.append(owner);
		line.append(" ");
		line.append(group);
		line.append(" ");
		line.append(sizestr);
		line.append(" ");
		line.append(filename);
		
		retval = "{listitem[";
		retval.append(1,type);
		retval.append(fancy_quote(filename));
		retval.append("] text:");
		retval.append(stfl::quote(line));
		retval.append("}");
	}
	return retval;
}

#if 0
std::string view::filebrowser(filebrowser_type type, const std::string& default_filename, std::string dir) {
	char cwdtmp[MAXPATHLEN];
	char buf[1024];
	::getcwd(cwdtmp,sizeof(cwdtmp));
	std::string cwd = cwdtmp;
	
	view_stack.push_front(&filebrowser_form);

	set_filebrowser_keymap_hint();

	filebrowser_form.set("fileprompt", _("File: "));
	
	bool update_list = true;
	bool quit = false;
	
	if (dir == "") {
		std::string save_path = cfg->get_configvalue("save-path");

		GetLogger().log(LOG_DEBUG,"view::filebrowser: save-path is '%s'",save_path.c_str());

		if (save_path.substr(0,2) == "~/") {
			char * homedir = ::getenv("HOME");
			if (homedir) {
				dir.append(homedir);
				dir.append(NEWSBEUTER_PATH_SEP);
				dir.append(save_path.substr(2,save_path.length()-2));
			} else {
				dir = ".";
			}
		} else {
			dir = save_path;
		}
	}

	GetLogger().log(LOG_DEBUG, "view::filebrowser: chdir(%s)", dir.c_str());
			
	::chdir(dir.c_str());
	::getcwd(cwdtmp,sizeof(cwdtmp));
	
	filebrowser_form.set("filenametext", default_filename);
	
	std::string head_str;
	if (type == FBT_OPEN) {
		snprintf(buf, sizeof(buf), _("Open File - %s"), cwdtmp);
	} else {
		snprintf(buf, sizeof(buf), _("Save File - %s"), cwdtmp);
	}
	head_str = buf;
	filebrowser_form.set("head", head_str);
		
	do {
		
		if (update_list) {
			std::string code = "{list";
			::getcwd(cwdtmp,sizeof(cwdtmp));
			
			DIR * dir = ::opendir(cwdtmp);
			if (dir) {
				struct dirent * de = ::readdir(dir);
				while (de) {
					if (strcmp(de->d_name,".")!=0)
						code.append(add_file(de->d_name));
					de = ::readdir(dir);
				}
				::closedir(dir);	
			}
			
			code.append("}");
			
			// std::cerr << "code: `" << code << "'" << std::endl;
			
			filebrowser_form.modify("files", "replace_inner", code);
			update_list = false;
		}
		
		const char * event = filebrowser_form.run(0);
		if (!event) continue;
		
		operation op = keys->get_operation(event);

		GetLogger().log(LOG_DEBUG,"view::filebrowser: event = %s operation = %d type = %d", event, op, type);
		
		switch (op) {
			case OP_OPEN: 
				{
					GetLogger().log(LOG_DEBUG,"view::filebrowser: 'opening' item");
					std::string focus = filebrowser_form.get_focus();
					if (focus.length() > 0) {
						if (focus == "files") {
							std::string selection = fancy_unquote(filebrowser_form.get("listposname"));
							char filetype = selection[0];
							selection.erase(0,1);
							std::string filename(selection);
							switch (filetype) {
								case 'd':
									if (type == FBT_OPEN) {
										snprintf(buf, sizeof(buf), _("Open File - %s"), filename.c_str());
									} else {
										snprintf(buf, sizeof(buf), _("Save File - %s"), filename.c_str());
									}
									head_str = buf;
									filebrowser_form.set("head", head_str);
									::chdir(filename.c_str());
									filebrowser_form.set("listpos","0");
									if (type == FBT_SAVE) {
										char cwdtmp[MAXPATHLEN];
										::getcwd(cwdtmp,sizeof(cwdtmp));
										std::string fn(cwdtmp);
										fn.append(NEWSBEUTER_PATH_SEP);
										std::string fnstr = filebrowser_form.get("filenametext");
										const char * base = strrchr(fnstr.c_str(),'/');
										if (!base)
											base = fnstr.c_str();
										fn.append(base);
										filebrowser_form.set("filenametext",fn);
									}
									update_list = true;
									break;
								case '-': 
									{
										char cwdtmp[MAXPATHLEN];
										::getcwd(cwdtmp,sizeof(cwdtmp));
										std::string fn(cwdtmp);
										fn.append(NEWSBEUTER_PATH_SEP);
										fn.append(filename);
										filebrowser_form.set("filenametext",fn);
										filebrowser_form.set_focus("filename");
									}
									break;
								default:
									// TODO: show error message
									break;
							}
						} else {
							std::string retval = filebrowser_form.get("filenametext");
							view_stack.pop_front();
							return retval;
						}
					}
				}
				break;
			case OP_QUIT:
				GetLogger().log(LOG_DEBUG,"view::filebrowser: quitting");
				view_stack.pop_front();
				return std::string("");
			default:
				break;
		}
		
		
	} while (!quit);
	view_stack.pop_front();
	return std::string(""); // never reached
}
#endif

#if 0
bool view::jump_to_next_unread_feed(bool begin_with_next) {
	std::string feedposname = feedlist_form.get("feedpos");
	unsigned int feedcount = visible_feeds.size();

	if (feedcount > 0 && feedposname.length() > 0) {
		std::istringstream posname(feedposname);
		unsigned int pos = 0;
		posname >> pos;
		for (unsigned int i=(begin_with_next?(pos+1):pos);i<feedcount;++i) {
			if (visible_feeds[i].first->unread_item_count() > 0) {
				std::ostringstream posname;
				posname << i;
				feedlist_form.set("feedpos", posname.str());
				GetLogger().log(LOG_DEBUG,"view::jump_to_next_unread_feed: jumped to pos %u", i);
				return true;
			}
		}
		for (unsigned int i=0;i<=pos;++i) {
			if (visible_feeds[i].first->unread_item_count() > 0) {
				std::ostringstream posname;
				posname << i;
				feedlist_form.set("feedpos", posname.str());
				GetLogger().log(LOG_DEBUG,"view::jump_to_next_unread_feed: jumped to pos %u (wraparound)", i);
				return true;
			}
		}
	} else {
		show_error(_("No feed selected!")); // shouldn't happen
	}
	GetLogger().log(LOG_DEBUG,"view::jump_to_next_unread_feed: no unread feeds");
	return false;
}

bool view::jump_to_next_unread_item(std::vector<rss_item>& items, bool begin_with_next) {
	std::string itemposname = itemlist_form.get("itempos");

	if (itemposname.length() > 0) {
		std::istringstream posname(itemposname);
		unsigned int pos = 0;
		posname >> pos;
		for (unsigned int i=(begin_with_next?(pos+1):pos);i<items.size();++i) {
			if (items[i].unread()) {
				std::ostringstream posname;
				posname << i;
				itemlist_form.set("itempos",posname.str());
				GetLogger().log(LOG_DEBUG,"view::jump_to_next_unread_item: jumped to pos %u", i);
				return true;
			}
		}
		for (unsigned int i=0;i<=pos;++i) {
			if (items[i].unread()) {
				std::ostringstream posname;
				posname << i;
				itemlist_form.set("itempos",posname.str());
				GetLogger().log(LOG_DEBUG,"view::jump_to_next_unread_item: jumped to pos %u (wraparound)", i);
				return true;
			}
		}
	} else {
		show_error(_("Error: no item selected!")); // shouldn't happen
	}
	return false;
}
#endif

#if 0
bool view::run_itemview(rss_feed& feed, std::string guid) {
	bool quit = false;
	bool show_source = false;
	bool redraw = true;
	bool retval = false;
	static bool render_hack = false;
	std::vector<linkpair> links;
	
	view_stack.push_front(&itemview_form);
	
	set_itemview_keymap_hint();
	itemview_form.set("msg","");

	do {
		if (redraw) {
			rss_item& item = feed.get_item_by_guid(guid);
			std::string code = "{list";

			code.append("{listitem text:");
			std::ostringstream feedtitle;
			feedtitle << _("Feed: ");
			if (feed.title().length() > 0) {
				feedtitle << feed.title();
			} else if (feed.link().length() > 0) {
				feedtitle << feed.link();
			} else if (feed.rssurl().length() > 0) {
				feedtitle << feed.rssurl();
			}
			code.append(stfl::quote(feedtitle.str().c_str()));
			code.append("}");

			code.append("{listitem text:");
			std::ostringstream title;
			title << _("Title: ");
			title << item.title();
			code.append(stfl::quote(title.str()));
			code.append("}");

			code.append("{listitem text:");
			std::ostringstream author;
			author << _("Author: ");
			author << item.author();
			code.append(stfl::quote(author.str()));
			code.append("}");

			code.append("{listitem text:");
			std::ostringstream link;
			link << _("Link: ");
			link << item.link();
			code.append(stfl::quote(link.str()));
			code.append("}");
			
			code.append("{listitem text:");
			std::ostringstream date;
			date << _("Date: ");
			date << item.pubDate();
			code.append(stfl::quote(date.str()));
			code.append("}");

			if (item.enclosure_url().length() > 0) {
				code.append("{listitem text:");
				std::ostringstream enc_url;
				enc_url << _("Podcast Download URL: ");
				enc_url << item.enclosure_url() << " (" << _("type: ") << item.enclosure_type() << ")";
				code.append(stfl::quote(enc_url.str()));
				code.append("}");
			}

			code.append("{listitem text:\"\"}");
			
			set_itemview_head(item.title());

			if (!render_hack) {
				itemview_form.run(-1); // XXX HACK: render once so that we get a proper widget width
				render_hack = true;
			}

			std::vector<std::string> lines;
			std::string widthstr = itemview_form.get("article:w");
			unsigned int render_width = 80;
			if (widthstr.length() > 0) {
				std::istringstream is(widthstr);
				is >> render_width;
				if (render_width - 5 > 0)
					render_width -= 5; 	
			}

			if (show_source) {
				render_source(lines, item.description(), render_width);
			} else {
				htmlrenderer rnd(render_width);
				rnd.render(item.description(), lines, links, item.feedurl());
			}

			for (std::vector<std::string>::iterator it = lines.begin(); it != lines.end(); ++it) {
				std::string line = std::string("{listitem text:") + stfl::quote(*it) + std::string("}");
				code.append(line);
			}

			code.append("}");

			itemview_form.modify("article","replace_inner",code);
			itemview_form.set("articleoffset","0");

			redraw = false;
		}

		const char * event = itemview_form.run(0);
		if (!event) continue;

		operation op = keys->get_operation(event);

		GetLogger().log(LOG_DEBUG, "view::run_itemview: event = %s operation = %d", event, op);

		rss_item& item = feed.get_item_by_guid(guid);

		switch (op) {
			case OP_OPEN:
				// nothing
				break;
			case OP_TOGGLESOURCEVIEW:
				GetLogger().log(LOG_INFO, "view::run_itemview: toggling source view");
				show_source = !show_source;
				redraw = true;
				break;
			case OP_ENQUEUE: {
					char buf[1024];
					snprintf(buf, sizeof(buf), _("Added %s to download queue."), item.enclosure_url().c_str());
					ctrl->enqueue_url(item.enclosure_url());
					set_status(buf);
				}
				break;
			case OP_SAVE:
				{
					char buf[1024];
					GetLogger().log(LOG_INFO, "view::run_itemview: saving article");
					std::string filename = filebrowser(FBT_SAVE,get_filename_suggestion(item.title()));
					if (filename == "") {
						show_error(_("Aborted saving."));	
					} else {
						try {
							write_item(item, filename);
							snprintf(buf, sizeof(buf), _("Saved article to %s."), filename.c_str());
							show_error(buf);
						} catch (...) {
							snprintf(buf, sizeof(buf), _("Error: couldn't write article to file %s"), filename.c_str());
							show_error(buf);
						}
					}
				}
				break;
			case OP_OPENINBROWSER:
				GetLogger().log(LOG_INFO, "view::run_itemview: starting browser");
				set_status(_("Starting browser..."));
				open_in_browser(item.link());
				set_status("");
				break;
			case OP_SHOWURLS:
				GetLogger().log(LOG_DEBUG, "view::run_itemview: showing URLs");
				run_urlview(links);
				break;
			case OP_NEXTUNREAD:
				GetLogger().log(LOG_INFO, "view::run_itemview: jumping to next unread article");
				retval = true; // fall-through is OK
			case OP_QUIT:
				GetLogger().log(LOG_INFO, "view::run_itemview: quitting");
				quit = true;
				break;
			case OP_HELP:
				run_help();
				set_status("");
				break;
			default:
				break;
		}

	} while (!quit);
	
	view_stack.pop_front();

	return retval;
}
#endif

#if 0
void view::open_in_browser(const std::string& url) {
	view_stack.push_front(NULL); // we don't want a thread to write over the browser
	std::string cmdline;
	std::string browser = cfg->get_configvalue("browser");
	if (browser != "")
		cmdline.append(browser);
	else
		cmdline.append("lynx");
	cmdline.append(" '");
	cmdline.append(url);
	cmdline.append("'");
	stfl::reset();
	GetLogger().log(LOG_DEBUG, "view::open_in_browser: running `%s'", cmdline.c_str());
	::system(cmdline.c_str());
	view_stack.pop_front();
}
#endif

#if 0
void view::run_urlview(std::vector<linkpair>& links) {
	set_urlview_keymap_hint();

	view_stack.push_front(&urlview_form);
	set_status("");

	urlview_form.set("head",_("URLs"));

	std::string code = "{list";
	unsigned int i=0;
	for (std::vector<linkpair>::iterator it = links.begin(); it != links.end(); ++it, ++i) {
		std::ostringstream os;
		char line[1024];
		snprintf(line,sizeof(line),"%2u  %s",i+1,it->first.c_str());
		os << "{listitem[" << i << "] text:" << stfl::quote(line) << "}";
		code.append(os.str());
	}
	code.append("}");

	urlview_form.modify("urls","replace_inner",code);

	bool quit = false;

	do {
		const char * event = urlview_form.run(0);
		if (!event) continue;

		operation op = keys->get_operation(event);

		switch (op) {
			case OP_OPEN: 
				{
					std::string posstr = urlview_form.get("feedpos");
					if (posstr.length() > 0) {
						std::istringstream is(posstr);
						unsigned int idx;
						is >> idx;
						set_status(_("Starting browser..."));
						open_in_browser(links[idx].first);
						set_status("");
					} else {
						show_error(_("No link selected!"));
					}
				}
				break;
			case OP_QUIT:
				quit = true;
				break;
			default: // nothing
				break;
		}
	} while (!quit);

	view_stack.pop_front();
}
#endif

#if 0
std::string view::select_tag(const std::vector<std::string>& tags) {
	std::string tag = "";

	set_selecttag_keymap_hint();

	view_stack.push_front(&selecttag_form);
	set_status("");

	std::string code = "{list";
	unsigned int i=0;
	for (std::vector<std::string>::const_iterator it=tags.begin();it!=tags.end();++it,++i) {
		std::ostringstream line;
		char num[32];
		snprintf(num,sizeof(num)," %4d. ", i+1);
		std::string tagstr = num;
		tagstr.append(it->c_str());
		line << "{listitem[" << i << "] text:" << stfl::quote(tagstr.c_str()) << "}";
		code.append(line.str());
	}
	code.append("}");

	selecttag_form.modify("taglist", "replace_inner", code);

	bool quit = false;
	
	do {
		const char * event = selecttag_form.run(0);
		if (!event) continue;

		operation op = keys->get_operation(event);

		switch (op) {
			case OP_QUIT:
				quit = true;
				break;
			case OP_OPEN: {
					std::string tagposname = selecttag_form.get("tagposname");
					if (tagposname.length() > 0) {
						std::istringstream posname(tagposname);
						unsigned int pos = 0;
						posname >> pos;
						if (pos < tags.size()) {
							tag = tags[pos];
							quit = true;
						}
					}
				}
				break;
			default:
				break;
		}
	} while (!quit);

	view_stack.pop_front();

	return tag;
}
#endif

#if 0
void view::run_help() {
	set_help_keymap_hint();

	view_stack.push_front(&help_form);
	set_status("");

	help_form.set("head",_("Help"));
	
	std::vector<std::pair<std::string,std::string> > descs;
	keys->get_keymap_descriptions(descs, KM_NEWSBEUTER);
	
	std::string code = "{list";
	
	for (std::vector<std::pair<std::string,std::string> >::iterator it=descs.begin();it!=descs.end();++it) {
		std::string line = "{listitem text:";
		std::string descline = it->first + std::string("\t") + it->second;
		line.append(stfl::quote(descline));
		line.append("}");
		
		code.append(line);
	}
	
	code.append("}");
	
	help_form.modify("helptext","replace_inner",code);
	
	bool quit = false;
	
	do {
		const char * event = help_form.run(0);
		if (!event) continue;

		operation op = keys->get_operation(event);

		switch (op) {
			case OP_QUIT:
				quit = true;
				break;
			default:
				break;
		}
	} while (!quit);
	
	view_stack.pop_front();
}
#endif

void view::set_feedlist(std::vector<rss_feed>& feeds) {
	feedlist->set_feedlist(feeds);
}

#if 0
std::string view::prepare_keymaphint(keymap_hint_entry * hints) {
	std::string keymap_hint;
	for (int i=0;hints[i].op != OP_NIL; ++i) {
		keymap_hint.append(keys->getkey(hints[i].op));
		keymap_hint.append(":");
		keymap_hint.append(hints[i].text);
		keymap_hint.append(" ");
	}
	return keymap_hint;	
}

void view::set_itemlist_keymap_hint() {
	keymap_hint_entry hints[] = {
		{ OP_QUIT, _("Quit") },
		{ OP_OPEN, _("Open") },
		{ OP_SAVE, _("Save") },
		{ OP_RELOAD, _("Reload") },
		{ OP_NEXTUNREAD, _("Next Unread") },
		{ OP_MARKFEEDREAD, _("Mark All Read") },
		{ OP_SEARCH, _("Search") },
		{ OP_HELP, _("Help") },
		{ OP_NIL, NULL }
	};
	std::string keymap_hint = prepare_keymaphint(hints);
	itemlist_form.set("help", keymap_hint);
}

void view::set_urlview_keymap_hint() {
	keymap_hint_entry hints[] = {
		{ OP_QUIT, _("Quit") },
		{ OP_OPEN, _("Open in Browser") },
		{ OP_NIL, NULL }
	};
	std::string keymap_hint = prepare_keymaphint(hints);
	urlview_form.set("help", keymap_hint);
}

void view::set_feedlist_keymap_hint() {
	keymap_hint_entry hints[] = {
		{ OP_QUIT, _("Quit") },
		{ OP_OPEN, _("Open") },
		{ OP_NEXTUNREAD, _("Next Unread") },
		{ OP_RELOAD, _("Reload") },
		{ OP_RELOADALL, _("Reload All") },
		{ OP_MARKFEEDREAD, _("Mark Read") },
		{ OP_MARKALLFEEDSREAD, _("Catchup All") },
		{ OP_SEARCH, _("Search") },
		{ OP_HELP, _("Help") },
		{ OP_NIL, NULL }
	};
	std::string keymap_hint = prepare_keymaphint(hints);
	feedlist_form.set("help", keymap_hint);
}

void view::set_filebrowser_keymap_hint() {
	keymap_hint_entry hints[] = {
		{ OP_QUIT, _("Cancel") },
		{ OP_OPEN, _("Save") },
		{ OP_NIL, NULL }
	};
	std::string keymap_hint = prepare_keymaphint(hints);
	filebrowser_form.set("help", keymap_hint);
}

void view::set_itemview_keymap_hint() {
	keymap_hint_entry hints[] = {
		{ OP_QUIT, _("Quit") },
		{ OP_OPEN, _("Open") },
		{ OP_SAVE, _("Save") },
		{ OP_NEXTUNREAD, _("Next Unread") },
		{ OP_OPENINBROWSER, _("Open in Browser") },
		{ OP_ENQUEUE, _("Enqueue") },
		{ OP_HELP, _("Help") },
		{ OP_NIL, NULL }
	};
	std::string keymap_hint = prepare_keymaphint(hints);
	itemview_form.set("help", keymap_hint);
}

void view::set_help_keymap_hint() {
	keymap_hint_entry hints[] = {
		{ OP_QUIT, _("Quit") },
		{ OP_NIL, NULL }
	};
	std::string keymap_hint = prepare_keymaphint(hints);
	help_form.set("help", keymap_hint);
}

void view::set_selecttag_keymap_hint() {
	keymap_hint_entry hints[] = {
		{ OP_QUIT, _("Cancel") },
		{ OP_OPEN, _("Select Tag") },
		{ OP_NIL, NULL }
	};

	std::string keymap_hint = prepare_keymaphint(hints);
	selecttag_form.set("help", keymap_hint);
}

void view::set_search_keymap_hint() {
	keymap_hint_entry hints[] = {
		{ OP_QUIT, _("Quit") },
		{ OP_OPEN, _("Search/Open") },
		{ OP_SEARCH, _("New Search") },
		{ OP_HELP, _("Help") },
		{ OP_NIL, NULL }
	};

	std::string keymap_hint = prepare_keymaphint(hints);
	search_form.set("help", keymap_hint);
}


void view::set_itemview_head(const std::string& s) {
	char buf[1024];
	snprintf(buf, sizeof(buf), _("Article '%s'"), s.c_str());
	itemview_form.set("head",buf);
}
#endif

void view::render_source(std::vector<std::string>& lines, std::string desc, unsigned int width) {
	std::string line;
	do {
		std::string::size_type pos = desc.find_first_of("\r\n");
		line = desc.substr(0,pos);
		if (pos == std::string::npos)
			desc.erase();
		else
			desc.erase(0,pos+1);
		while (line.length() > width) {
			int i = width;
			while (i > 0 && line[i] != ' ' && line[i] != '<')
				--i;
			if (0 == i) {
				i = width;
			}
			std::string subline = line.substr(0, i);
			line.erase(0, i);
			pos = subline.find_first_not_of(" ");
			subline.erase(0,pos);
			lines.push_back(subline);
		}
		pos = line.find_first_not_of(" ");
		line.erase(0,pos);
		lines.push_back(line);
	} while (desc.length() > 0);
}

void view::set_tags(const std::vector<std::string>& t) {
	feedlist->set_tags(t);
}

void view::push_itemlist(unsigned int pos) {
	rss_feed * feed = ctrl->get_feed(pos);
	GetLogger().log(LOG_DEBUG, "view::push_itemlist: retrieved feed at position %d (address = %p)", pos, feed);
	itemlist->set_feed(feed);
	itemlist->set_pos(pos);
	itemlist->init();
	formaction_stack.push_front(itemlist);
}

void view::pop_current_formaction() {
	formaction_stack.pop_front();
	if (formaction_stack.size() > 0) {
		(*formaction_stack.begin())->set_redraw(true);
	}
}

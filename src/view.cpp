#include <feedlist.h>
#include <itemlist.h>
#include <itemview.h>
#include <help.h>
#include <filebrowser.h>
#include <urlview.h>
#include <selecttag.h>

#include <logger.h>
#include <reloadthread.h>

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

view::view(controller * c) : ctrl(c), cfg(0), keys(0), mtx(0),
		feedlist_form(feedlist_str), itemlist_form(itemlist_str), itemview_form(itemview_str), 
		help_form(help_str), filebrowser_form(filebrowser_str), urlview_form(urlview_str), selecttag_form(selecttag_str) { 
	mtx = new mutex();
}

view::~view() {
	stfl::reset();
	delete mtx;
}

void view::set_config_container(configcontainer * cfgcontainer) {
	cfg = cfgcontainer;	
}

void view::set_keymap(keymap * k) {
	keys = k;
}

void view::set_status(const char * msg) {
	mtx->lock();
	if (view_stack.size() > 0) {
		stfl::form * form = *(view_stack.begin());
		if (form) {
			form->set("msg",msg);
			form->run(-1);
		}
	}
	mtx->unlock();
}

void view::show_error(const char * msg) {
	set_status(msg);
}

void view::run_feedlist(const std::vector<std::string>& tags) {
	bool quit = false;
	bool update = false;
	bool zero_feedpos = false;

	view_stack.push_front(&feedlist_form);
	
	set_feedlist_keymap_hint();

	if(ctrl->get_refresh_on_start()) {
		feedlist_form.run(-1);
		ctrl->start_reload_all_thread();
	}

	unsigned int reload_cycle = 60 * static_cast<unsigned int>(cfg->get_configvalue_as_int("reload-time"));
	if (cfg->get_configvalue_as_bool("auto-reload") == true) {
		reloadthread  * rt = new reloadthread(ctrl, reload_cycle);
		rt->start();
	}

	do {

		if (update) {
			update = false;
			ctrl->update_feedlist();
			if (zero_feedpos) {
				feedlist_form.set("feedpos","0");
				zero_feedpos = false;
			}
		}

		const char * event = feedlist_form.run(0);
		if (!event) continue;

		operation op = keys->get_operation(event);

		GetLogger().log(LOG_DEBUG,"view::run_feedlist: event = %s operation = %d", event, op);

		switch (op) {
			case OP_OPEN: {
					bool quit = false;
					bool auto_open = false;
					do {
						std::string feedpos = feedlist_form.get("feedpos");
						GetLogger().log(LOG_INFO, "view::run_feedlist: opening feed at position `%s'",feedpos.c_str());
						if (feeds_shown > 0 && feedpos.length() > 0) {
							std::istringstream posname(feedpos);
							unsigned int pos = 0;
							posname >> pos;
							if ((auto_open = ctrl->open_feed(visible_feeds[pos].second, auto_open))) {
								if (!jump_to_next_unread_feed()) {
									show_error("No feeds with unread items.");
									quit = true;
								}
							} else {
								quit = true;
							}
						} else {
							show_error("No feed selected!"); // should not happen
						}
					} while (!quit);
				}
				break;
			case OP_RELOAD: {
					std::string feedposname = feedlist_form.get("feedposname");
					GetLogger().log(LOG_INFO, "view::run_feedlist: reloading feed at position `%s'",feedposname.c_str());
					if (feeds_shown > 0 && feedposname.length() > 0) {
						std::istringstream posname(feedposname);
						unsigned int pos = 0;
						posname >> pos;
						ctrl->reload(pos);
					} else {
						show_error("No feed selected!"); // should not happen
					}
				}
				break;
			case OP_RELOADALL:
				GetLogger().log(LOG_INFO, "view::run_feedlist: reloading all feeds");
				ctrl->start_reload_all_thread();
				break;
			case OP_MARKFEEDREAD: {
					std::string feedposname = feedlist_form.get("feedposname");
					GetLogger().log(LOG_INFO, "view::run_feedlist: marking feed read at position `%s'",feedposname.c_str());
					if (feeds_shown > 0 && feedposname.length() > 0) {
						set_status("Marking feed read...");
						std::istringstream posname(feedposname);
						unsigned int pos = 0;
						posname >> pos;
						ctrl->mark_all_read(pos);
						update = true;
						set_status("");
					} else {
						show_error("No feed selected!"); // should not happen
					}
				}
				break;
			case OP_TOGGLESHOWREAD:
				GetLogger().log(LOG_INFO, "view::run_feedlist: toggling show-read-feeds");
				if (cfg->get_configvalue_as_bool("show-read-feeds")) {
					cfg->set_configvalue("show-read-feeds","no");
				} else {
					cfg->set_configvalue("show-read-feeds","yes");
				}
				update = true;
				break;
			case OP_NEXTUNREAD:
				GetLogger().log(LOG_INFO, "view::run_feedlist: jumping to next unred feed");
				if (!jump_to_next_unread_feed()) {
					show_error("No feeds with unread items.");
				}
				break;
			case OP_MARKALLFEEDSREAD:
				GetLogger().log(LOG_INFO, "view::run_feedlist: marking all feeds read");
				set_status("Marking all feeds read...");
				ctrl->catchup_all();
				set_status("");
				update = true;
				break;
			case OP_CLEARTAG:
				tag = "";
				update = true;
				zero_feedpos = true;
				break;
			case OP_SETTAG: 
				if (tags.size() > 0) {
					std::string newtag = select_tag(tags);
					if (newtag != "") {
						tag = newtag;
						update = true;
						zero_feedpos = true;
					}
				} else {
					show_error("No tags defined.");
				}
				break;
			case OP_QUIT:
				GetLogger().log(LOG_INFO, "view::run_feedlist: quitting");
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

	// delete rt; // is this allowed?

	stfl::reset();
}

bool view::run_itemlist(unsigned int pos, bool auto_open) {
	bool quit = false;
	bool rebuild_list = true;
	bool show_no_unread_error = false;
	bool retval = false;
	rss_feed& feed = ctrl->get_feed(pos);
	std::vector<rss_item>& items = feed.items();
	
	view_stack.push_front(&itemlist_form);

	itemlist_form.set("itempos","0");
	
	set_itemlist_keymap_hint();

	itemlist_form.set("msg","");
	
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

			itemlist_form.modify("items","replace_inner",code);
			
			set_itemlist_head(feed.title(),feed.unread_item_count(),feed.items().size(), feed.rssurl());

			rebuild_list = false;
		}
		
		if (show_no_unread_error) {
			show_error("No unread items.");
			show_no_unread_error = false;
		}

		operation op;
		const char * event = NULL;
		
		if (auto_open) {
			auto_open = false;
			jump_to_next_unread_item(items);
			op = OP_OPEN;
		} else {
			event = itemlist_form.run(0);
			if (!event) continue;
			op = keys->get_operation(event);
		}

		GetLogger().log(LOG_DEBUG, "view::run_itemlist: event = %s operation = %d auto_open = %d", event, op, auto_open);

		switch (op) {
			case OP_OPEN: {
					bool open_next_item = false;
					do {
						std::string itemposname = itemlist_form.get("itempos");
						GetLogger().log(LOG_INFO, "view::run_itemlist: opening item at pos `%s' open_next_item = %d", itemposname.c_str(), open_next_item);
						if (itemposname.length() > 0) {
							std::istringstream posname(itemposname);
							unsigned int pos = 0;
							posname >> pos;
							open_next_item = ctrl->open_item(feed, items[pos]);
							rebuild_list = true;
						} else {
							show_error("No item selected!"); // should not happen
						}
						if (open_next_item) {
							if (!jump_to_next_unread_item(items)) {
								open_next_item = false;
								retval = true;
								quit = true;
							}
						}
					} while (open_next_item);
					// set_status("");
				}
				break;
			case OP_SAVE: 
				{
					std::string itemposname = itemlist_form.get("itempos");
					GetLogger().log(LOG_INFO, "view::run_itemlist: saving item at pos `%s'", itemposname.c_str());
					if (itemposname.length() > 0) {
						std::istringstream posname(itemposname);
						unsigned int pos = 0;
						posname >> pos;
						
						std::string filename = filebrowser(FBT_SAVE,get_filename_suggestion(items[pos].title()));
						if (filename == "") {
							show_error("Aborted saving.");	
						} else {
							// TODO: render and save
							try {
								write_item(items[pos], filename);
								std::ostringstream msg;
								msg << "Saved article to " << filename;
								show_error(msg.str().c_str());
							
							} catch (...) {
								std::ostringstream msg;
								msg << "Error: couldn't save article to " << filename;
								show_error(msg.str().c_str());	
							}
						}
					} else {
						show_error("Error: no item selected!");
					}
				}
				break;
			case OP_HELP:
				run_help();
				set_status("");
				break;
			case OP_RELOAD:
				GetLogger().log(LOG_INFO, "view::run_itemlist: reloading current feed");
				ctrl->reload(pos);
				feed = ctrl->get_feed(pos);
				rebuild_list = true;
				break;
			case OP_QUIT:
				GetLogger().log(LOG_INFO, "view::run_itemlist: quitting");
				quit = true;
				break;
			case OP_NEXTUNREAD:
				GetLogger().log(LOG_INFO, "view::run_itemlist: jumping to next unread item");
				if (!jump_to_next_unread_item(items))
					show_no_unread_error = true;
				break;
			case OP_MARKFEEDREAD:
				GetLogger().log(LOG_INFO, "view::run_itemlist: marking feed read");
				set_status("Marking feed read...");
				mark_all_read(items);
				set_status("");
				rebuild_list = true;
				break;
			case OP_TOGGLEITEMREAD: {
					std::string itemposname = itemlist_form.get("itempos");
					GetLogger().log(LOG_INFO, "view::run_itemlist: toggling item read at pos `%s'", itemposname.c_str());
					if (itemposname.length() > 0) {
						std::istringstream posname(itemposname);
						unsigned int pos = 0;
						posname >> pos;
						set_status("Toggling read flag for article...");
						items[pos].set_unread(!items[pos].unread());
						set_status("");
						rebuild_list = true;
					}
				}
				break;
			default:
				break;
		}

	} while (!quit);
	
	view_stack.pop_front();

	return retval;
}

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
	std::vector<std::string> links; // not used
	
	std::string title("Title: ");
	title.append(item.title());
	lines.push_back(title);
	
	std::string author("Author: ");
	author.append(item.author());
	lines.push_back(author);
	
	std::string date("Date: ");
	date.append(item.pubDate());
	lines.push_back(date);
	
	lines.push_back(std::string(""));
	
	htmlrenderer rnd(80);
	rnd.render(item.description(), lines, links);

	std::fstream f;
	f.open(filename.c_str(),std::fstream::out);
	if (!f.is_open())
		throw 1; // TODO: add real exception with real error message and such
		
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

std::string view::filebrowser(filebrowser_type type, const std::string& default_filename, std::string dir) {
	char cwdtmp[MAXPATHLEN];
	::getcwd(cwdtmp,sizeof(cwdtmp));
	std::string cwd = cwdtmp;
	
	view_stack.push_front(&filebrowser_form);

	set_filebrowser_keymap_hint();
	
	bool update_list = true;
	bool quit = false;
	
	if (dir == "") {
		char * homedir = ::getenv("HOME");
		if (homedir)
			dir = homedir;
		else
			dir = ".";
	}
			
	::chdir(dir.c_str());
	
	filebrowser_form.set("filenametext", default_filename);
	
	std::string head_str;
	if (type == FBT_OPEN) {
		head_str = "Open File - ";
	} else {
		head_str = "Save File - ";
	}
	head_str.append(dir);
	filebrowser_form.set("head", head_str);
		
	do {
		
		if (update_list) {
			std::string code = "{list";
			// TODO: read from current directory
			char cwdtmp[MAXPATHLEN];
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
									// TODO: handle directory
									if (type == FBT_OPEN) {
										head_str = "Open File - ";
									} else {
										head_str = "Save File - ";
									}
									head_str.append(filename);
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

bool view::jump_to_next_unread_feed() {
	std::string feedposname = feedlist_form.get("feedpos");
	unsigned int feedcount = visible_feeds.size();

	if (feedcount > 0 && feedposname.length() > 0) {
		std::istringstream posname(feedposname);
		unsigned int pos = 0;
		posname >> pos;
		for (unsigned int i=pos+1;i<feedcount;++i) {
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
		show_error("No feed selected!"); // shouldn't happen
	}
	GetLogger().log(LOG_DEBUG,"view::jump_to_next_unread_feed: no unread feeds");
	return false;
}

bool view::jump_to_next_unread_item(std::vector<rss_item>& items) {
	std::string itemposname = itemlist_form.get("itempos");

	if (itemposname.length() > 0) {
		std::istringstream posname(itemposname);
		unsigned int pos = 0;
		posname >> pos;
		for (unsigned int i=pos+1;i<items.size();++i) {
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
		show_error("Error: no item selected!"); // shouldn't happen
	}
	return false;
}

bool view::run_itemview(const rss_feed& feed, rss_item& item) {
	bool quit = false;
	bool show_source = false;
	bool redraw = true;
	bool retval = false;
	static bool render_hack = false;
	std::vector<std::string> links;
	
	view_stack.push_front(&itemview_form);
	
	set_itemview_keymap_hint();
	itemview_form.set("msg","");

	do {
		if (redraw) {
			std::string code = "{list";

			code.append("{listitem text:");
			std::ostringstream feedtitle;
			feedtitle << "Feed: ";
			if (feed.title().length() > 0) {
				feedtitle << feed.title();
			} else if (feed.link().length() > 0) {
				feedtitle << feed.link();
			} else if (feed.rssurl().length() > 0) {
				feedtitle << feed.rssurl();
			}
			code.append(stfl_quote(feedtitle.str().c_str()));
			code.append("}");

			code.append("{listitem text:");
			std::ostringstream title;
			title << "Title: ";
			title << item.title();
			code.append(stfl::quote(title.str()));
			code.append("}");

			code.append("{listitem text:");
			std::ostringstream author;
			author << "Author: ";
			author << item.author();
			code.append(stfl::quote(author.str()));
			code.append("}");

			code.append("{listitem text:");
			std::ostringstream link;
			link << "Link: ";
			link << item.link();
			code.append(stfl::quote(link.str()));
			code.append("}");
			
			code.append("{listitem text:");
			std::ostringstream date;
			date << "Date: ";
			date << item.pubDate();
			code.append(stfl::quote(date.str()));
			code.append("}");

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
				rnd.render(item.description(), lines, links);
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

		switch (op) {
			case OP_OPEN:
				// nothing
				break;
			case OP_TOGGLESOURCEVIEW:
				GetLogger().log(LOG_INFO, "view::run_itemview: toggling source view");
				show_source = !show_source;
				redraw = true;
				break;
			case OP_SAVE:
				{
					GetLogger().log(LOG_INFO, "view::run_itemview: saving article");
					std::string filename = filebrowser(FBT_SAVE,get_filename_suggestion(item.title()));
					if (filename == "") {
						show_error("Aborted saving.");	
					} else {
						try {
							write_item(item, filename);
							std::ostringstream msg;
							msg << "Saved article to " << filename;
							show_error(msg.str().c_str());
						} catch (...) {
							std::ostringstream msg;
							msg << "Error: couldn't write article to file " << filename;
							show_error(msg.str().c_str());
						}
					}
				}
				break;
			case OP_OPENINBROWSER:
				GetLogger().log(LOG_INFO, "view::run_itemview: starting browser");
				set_status("Starting browser...");
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

void view::run_urlview(std::vector<std::string>& links) {
	set_urlview_keymap_hint();

	view_stack.push_front(&urlview_form);
	set_status("");

	std::string code = "{list";
	unsigned int i=0;
	for (std::vector<std::string>::iterator it = links.begin(); it != links.end(); ++it, ++i) {
		std::ostringstream os;
		char line[1024];
		snprintf(line,sizeof(line),"%2u  %s",i+1,it->c_str());
		os << "{listitem[" << i << "] text:" << stfl_quote(line) << "}";
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
						set_status("Starting browser...");
						open_in_browser(links[idx]);
						set_status("");
					} else {
						show_error("No link selected!");
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
		line << "{listitem[" << i << "] text:" << stfl_quote(tagstr.c_str()) << "}";
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

void view::run_help() {
	set_help_keymap_hint();

	view_stack.push_front(&help_form);
	set_status("");
	
	std::vector<std::pair<std::string,std::string> > descs;
	keys->get_keymap_descriptions(descs);
	
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

void view::set_feedlist(std::vector<rss_feed>& feeds) {
	std::string code = "{list";
	
	assert(cfg != NULL); // must not happen
	
	bool show_read_feeds = cfg->get_configvalue_as_bool("show-read-feeds");
	
	// std::cerr << "show-read-feeds" << (show_read_feeds?"true":"false") << std::endl;

	feeds_shown = 0;
	unsigned int i = 0;
	unsigned short feedlist_number = 1;
	unsigned int unread_feeds = 0;

	if (visible_feeds.size() > 0)
		visible_feeds.erase(visible_feeds.begin(), visible_feeds.end());

	for (std::vector<rss_feed>::iterator it = feeds.begin(); it != feeds.end(); ++it, ++i, ++feedlist_number) {
		rss_feed feed = *it;
		std::string title = it->title();
		if (title.length()==0) {
			title = it->rssurl(); // rssurl must always be present.
			if (title.length()==0) {
				title = "<no title>"; // shouldn't happen
			}
		}

		// TODO: refactor
		char buf[20];
		char buf2[20];
		unsigned int unread_count = 0;
		if (it->items().size() > 0) {
			for (std::vector<rss_item>::iterator rit = it->items().begin(); rit != it->items().end(); ++rit) {
				if (rit->unread())
					++unread_count;
			}
		}
		if (unread_count > 0)
			++unread_feeds;


		if ((tag == "" || it->matches_tag(tag)) && (show_read_feeds || unread_count > 0)) {
			visible_feeds.push_back(std::pair<rss_feed *, unsigned int>(&(*it),i));

			snprintf(buf,sizeof(buf),"(%u/%u) ",unread_count,static_cast<unsigned int>(it->items().size()));
			snprintf(buf2,sizeof(buf2),"%4u %c %11s",feedlist_number, unread_count > 0 ? 'N' : ' ',buf);
			std::string newtitle(buf2);
			newtitle.append(title);
			title = newtitle;

			std::string line = "{listitem[";
			std::ostringstream num;
			num << i;
			line.append(num.str());
			line.append("] text:");
			line.append(stfl::quote(title));
			line.append("}");

			code.append(line);

			++feeds_shown;
		}
	}

	code.append("}");

	feedlist_form.modify("feeds","replace_inner",code);

	std::ostringstream titleos;

	titleos << "Your feeds (" << unread_feeds << " unread, " << i << " total)";

	if (tag.length() > 0) {
		titleos << " - tag `" << tag << "'";
	}

	feedlist_form.set("head", titleos.str());
}

void view::mark_all_read(std::vector<rss_item>& items) {
	for (std::vector<rss_item>::iterator it = items.begin(); it != items.end(); ++it) {
		it->set_unread(false);
	}
}

struct keymap_hint_entry {
	operation op; 
	char * text;
};

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
		{ OP_QUIT, "Quit" },
		{ OP_OPEN, "Open" },
		{ OP_SAVE, "Save" },
		{ OP_RELOAD, "Reload" },
		{ OP_NEXTUNREAD, "Next Unread" },
		{ OP_MARKFEEDREAD, "Mark All Read" },
		{ OP_HELP, "Help" },
		{ OP_NIL, NULL }
	};
	std::string keymap_hint = prepare_keymaphint(hints);
	itemlist_form.set("help", keymap_hint);
}

void view::set_urlview_keymap_hint() {
	keymap_hint_entry hints[] = {
		{ OP_QUIT, "Quit" },
		{ OP_OPEN, "Open in Browser" },
		{ OP_NIL, NULL }
	};
	std::string keymap_hint = prepare_keymaphint(hints);
	itemlist_form.set("help", keymap_hint);
}

void view::set_feedlist_keymap_hint() {
	keymap_hint_entry hints[] = {
		{ OP_QUIT, "Quit" },
		{ OP_OPEN, "Open" },
		{ OP_NEXTUNREAD, "Next Unread" },
		{ OP_RELOAD, "Reload" },
		{ OP_RELOADALL, "Reload All" },
		{ OP_MARKFEEDREAD, "Mark Read" },
		{ OP_MARKALLFEEDSREAD, "Catchup All" },
		{ OP_HELP, "Help" },
		{ OP_NIL, NULL }
	};
	std::string keymap_hint = prepare_keymaphint(hints);
	feedlist_form.set("help", keymap_hint);
}

void view::set_filebrowser_keymap_hint() {
	keymap_hint_entry hints[] = {
		{ OP_QUIT, "Cancel" },
		{ OP_OPEN, "Save" },
		{ OP_NIL, NULL }
	};
	std::string keymap_hint = prepare_keymaphint(hints);
	filebrowser_form.set("help", keymap_hint);
}

void view::set_itemview_keymap_hint() {
	keymap_hint_entry hints[] = {
		{ OP_QUIT, "Quit" },
		{ OP_OPEN, "Open" },
		{ OP_SAVE, "Save" },
		{ OP_NEXTUNREAD, "Next Unread" },
		{ OP_OPENINBROWSER, "Open in Browser" },
		{ OP_HELP, "Help" },
		{ OP_NIL, NULL }
	};
	std::string keymap_hint = prepare_keymaphint(hints);
	itemview_form.set("help", keymap_hint);
}

void view::set_help_keymap_hint() {
	keymap_hint_entry hints[] = {
		{ OP_QUIT, "Quit" },
		{ OP_NIL, NULL }
	};
	std::string keymap_hint = prepare_keymaphint(hints);
	help_form.set("help", keymap_hint);
}

void view::set_selecttag_keymap_hint() {
	keymap_hint_entry hints[] = {
		{ OP_QUIT, "Cancel" },
		{ OP_OPEN, "Select Tag" },
		{ OP_NIL, NULL }
	};

	std::string keymap_hint = prepare_keymaphint(hints);
	selecttag_form.set("help", keymap_hint);
}

void view::set_itemlist_head(const std::string& s, unsigned int unread, unsigned int total, const std::string &url) {
	std::ostringstream caption;
	
	caption << "Articles in feed '" << s << "' (" << unread << " unread, " << total << " total) " << "- " << url;
	itemlist_form.set("head",caption.str());
}

void view::set_itemview_head(const std::string& s) {
	std::string caption = "Article '";
	caption.append(s);
	caption.append("'");
	itemview_form.set("head",caption);
}

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


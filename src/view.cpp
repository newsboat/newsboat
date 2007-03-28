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
#include <itemview_formaction.h>
#include <help_formaction.h>
#include <urlview_formaction.h>
#include <selecttag_formaction.h>
#include <search_formaction.h>

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
	itemview = new itemview_formaction(this, itemview_str);
	helpview = new help_formaction(this, help_str);
	filebrowser = new filebrowser_formaction(this, filebrowser_str);
	urlview = new urlview_formaction(this, urlview_str);
	selecttag = new selecttag_formaction(this, selecttag_str);
	search = new search_formaction(this, search_str);

	// push the dialog to start with onto the stack
	formaction_stack.push_front(feedlist);
}

view::~view() {
	stfl::reset();
	delete mtx;
	delete feedlist;
	delete itemlist;
	delete itemview;
	delete helpview;
	delete filebrowser;
	delete urlview;
	delete selecttag;
	delete search;
}

void view::set_config_container(configcontainer * cfgcontainer) {
	cfg = cfgcontainer;	
}

void view::set_keymap(keymap * k) {
	keys = k;
}

void view::set_status(const char * msg) {
	mtx->lock();
	if (formaction_stack.size() > 0 && (*formaction_stack.begin()) != NULL) {
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

std::string view::run_modal(formaction * f, const std::string& value) {
	f->init();
	unsigned int stacksize = formaction_stack.size();

	formaction_stack.push_front(f);

	while (formaction_stack.size() > stacksize) {
		formaction * fa = *(formaction_stack.begin());

		fa->prepare();

		const char * event = fa->get_form().run(0);
		if (!event) continue;

		operation op = keys->get_operation(event);

		fa->process_operation(op);
	}

	if (value == "")
		return "";
	else
		return f->get_value(value);
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

void view::open_in_browser(const std::string& url) {
	formaction_stack.push_front(NULL); // we don't want a thread to write over the browser
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
	formaction_stack.pop_front();
}

void view::set_feedlist(std::vector<rss_feed>& feeds) {
	feedlist->set_feedlist(feeds);
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

void view::push_itemview(rss_feed * f, const std::string& guid) {
	itemview->set_feed(f);
	itemview->set_guid(guid);
	itemview->init();
	formaction_stack.push_front(itemview);
}

void view::push_help() {
	formaction_stack.push_front(helpview);
}

void view::push_urlview(const std::vector<linkpair>& links) {
	urlview->set_links(links);
	formaction_stack.push_front(urlview);
}

std::string view::run_filebrowser(filebrowser_type type, const std::string& default_filename, const std::string& dir) {
	filebrowser->set_dir(dir);
	filebrowser->set_default_filename(default_filename);
	filebrowser->set_type(type);
	return run_modal(filebrowser, "filenametext");
}

std::string view::select_tag(const std::vector<std::string>& tags) {
	selecttag->set_tags(tags);
	run_modal(selecttag, "");
	return selecttag->get_tag();
}

void view::run_search(const std::string& feedurl) {
	search->set_feedurl(feedurl);
	search->init();
	formaction_stack.push_front(search);
}

void view::pop_current_formaction() {
	formaction_stack.pop_front();
	if (formaction_stack.size() > 0) {
		(*formaction_stack.begin())->set_redraw(true);
	}
}


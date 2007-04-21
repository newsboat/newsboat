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

view::view(controller * c) : ctrl(c), cfg(0), keys(0), mtx(0) {
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
	GetLogger().log(LOG_DEBUG, "view::set_status: after mtx->lock; formaction_stack.size = %u", formaction_stack.size());
	if (formaction_stack.size() > 0 && (*formaction_stack.begin()) != NULL) {
		stfl::form * form = (*formaction_stack.begin())->get_form();
		GetLogger().log(LOG_DEBUG, "view::set_status: form = %p", form);
		form->set("msg",msg);
		GetLogger().log(LOG_DEBUG, "view::set_status: after form.set");
		form->run(-1);
		GetLogger().log(LOG_DEBUG, "view::set_status: after form.run");
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

		const char * event = fa->get_form()->run(1000);
		if (!event || strcmp(event,"TIMEOUT")==0) continue;

		operation op = keys->get_operation(event);

		if (OP_REDRAW == op) {
			stfl::reset();
			continue;
		}

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

		const char * event = fa->get_form()->run(1000);
		if (!event || strcmp(event,"TIMEOUT")==0) continue;

		operation op = keys->get_operation(event);

		if (OP_REDRAW == op) {
			stfl::reset();
			continue;
		}

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
	mtx->lock();
	feedlist->set_feedlist(feeds);
	mtx->unlock();
}


void view::set_tags(const std::vector<std::string>& t) {
	feedlist->set_tags(t);
}

void view::push_itemlist(unsigned int pos) {
	rss_feed * feed = ctrl->get_feed(pos);
	GetLogger().log(LOG_DEBUG, "view::push_itemlist: retrieved feed at position %d (address = %p)", pos, feed);
	if (feed->items().size() > 0) {
		itemlist->set_feed(feed);
		itemlist->set_pos(pos);
		itemlist->init();
		formaction_stack.push_front(itemlist);
	} else {
		show_error(_("Error: feed contains no items!"));
	}
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

char view::confirm(const std::string& prompt, const std::string& charset) {
	formaction * f = *formaction_stack.begin();
	formaction_stack.push_front(NULL);
	f->get_form()->set("msg", prompt);

	char result = 0;

	do {
		const char * event = f->get_form()->run(0);
		if (!event) continue;
		result = keys->get_key(event);
	} while (result && strchr(charset.c_str(), result)==NULL);

	f->get_form()->set("msg", "");
	f->get_form()->run(-1);

	formaction_stack.pop_front(); // remove the NULL pointer from the formaction stack

	return result;
}

bool view::get_next_unread() {
	if (itemlist->jump_to_next_unread_item(false)) {
		itemview->init();
		itemview->set_feed(itemlist->get_feed());
		itemview->set_guid(itemlist->get_guid());
		return true;
	} else if (feedlist->jump_to_next_unread_feed()) {
		itemlist->set_feed(feedlist->get_feed());
		itemlist->init();
		if (itemlist->jump_to_next_unread_item(true)) {
			itemview->init();
			itemview->set_feed(itemlist->get_feed());
			itemview->set_guid(itemlist->get_guid());
			return true;
		}
	}
	return false;
}

void view::pop_current_formaction() {
	formaction_stack.pop_front();
	if (formaction_stack.size() > 0) {
		formaction * f = (*formaction_stack.begin());
		if (f) {
			f->set_redraw(true);
			f->get_form()->set("msg","");
		}
	}
}


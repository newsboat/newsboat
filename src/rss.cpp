#include <rss.h>
#include <config.h>
#include <cache.h>
#include <tagsouppullparser.h>
#include <utils.h>
#include <logger.h>
#include <exceptions.h>
#include <sstream>
#include <iostream>
#include <configcontainer.h>
#include <cstring>
#include <algorithm>
#include <curl/curl.h>
#include <sys/utsname.h>
#include <htmlrenderer.h>

#include <langinfo.h>

#include <cerrno>

#include <functional>

namespace newsbeuter {

rss_item::rss_item(cache * c) : unread_(true), ch(c), enqueued_(false), deleted_(0), idx(0), override_unread_(false), size_(0) {
	// LOG(LOG_CRITICAL, "new rss_item");
}

rss_item::~rss_item() {
	// LOG(LOG_CRITICAL, "delete rss_item");
}

rss_feed::rss_feed(cache * c) : ch(c), empty(true), is_rtl_(false), idx(0), status_(SUCCESS) {
	// LOG(LOG_CRITICAL, "new rss_feed");
}

rss_feed::~rss_feed() {
	// LOG(LOG_CRITICAL, "delete rss_feed");
	clear_items();
}

// rss_item setters

void rss_item::set_title(const std::string& t) { 
	title_ = t; 
	utils::trim(title_);
}


void rss_item::set_link(const std::string& l) { 
	link_ = l; 
	utils::trim(link_);
}

void rss_item::set_author(const std::string& a) { 
	author_ = a; 
}

void rss_item::set_description(const std::string& d) { 
	description_ = d; 
}

void rss_item::set_size(unsigned int size) {
	size_ = size;
}

std::string rss_item::length() const {
	std::string::size_type l(size_);
	if (!l)
		return "";
	if (l < 1000)
	return utils::strprintf("%u ", l);
	if (l < 1024*1000)
		return utils::strprintf("%.1fK", l/1024.0);

	return utils::strprintf("%.1fM", l/1024.0/1024.0);
}

void rss_item::set_pubDate(time_t t) { 
	pubDate_ = t; 
}

void rss_item::set_guid(const std::string& g) { 
	guid_ = g; 
}

void rss_item::set_unread_nowrite(bool u) {
	unread_ = u;
}

void rss_item::set_unread_nowrite_notify(bool u, bool notify) {
	unread_ = u;
	if (feedptr && notify) {
		feedptr->get_item_by_guid(guid_)->set_unread_nowrite(unread_); // notify parent feed
	}
}

void rss_item::set_unread(bool u) { 
	if (unread_ != u) {
		bool old_u = unread_;
		unread_ = u;
		if (feedptr)
			feedptr->get_item_by_guid(guid_)->set_unread_nowrite(unread_); // notify parent feed
		try {
			if (ch) ch->update_rssitem_unread_and_enqueued(this, feedurl_); 
		} catch (const dbexception& e) {
			// if the update failed, restore the old unread flag and rethrow the exception
			unread_ = old_u; 
			throw;
		}
	}
}

std::string rss_item::pubDate() const {
	char text[1024];
	strftime(text,sizeof(text),"%a, %d %b %Y %T %z", localtime(&pubDate_)); 
	return std::string(text);
}

unsigned int rss_feed::unread_item_count() {
	scope_mutex lock(&item_mutex);
	unsigned int count = 0;
	for (std::vector<std::tr1::shared_ptr<rss_item> >::const_iterator it=items_.begin();it!=items_.end();++it) {
		if ((*it)->unread())
			++count;
	}
	return count;
}


bool rss_feed::matches_tag(const std::string& tag) {
	for (std::vector<std::string>::iterator it=tags_.begin();it!=tags_.end();++it) {
		if (tag == *it)
			return true;
	}
	return false;
}

std::string rss_feed::get_firsttag() {
	if (tags_.size() == 0)
		return "";
	return tags_[0];
}

std::string rss_feed::get_tags() {
	std::string tags;
	for (std::vector<std::string>::iterator it=tags_.begin();it!=tags_.end();++it) {
		if (it->substr(0,1) != "~" && it->substr(0,1) != "!") {
			tags.append(*it);
			tags.append(" ");
		}
	}
	return tags;
}

void rss_feed::set_tags(const std::vector<std::string>& tags) {
	tags_.clear();
	for (std::vector<std::string>::const_iterator it=tags.begin();it!=tags.end();++it) {
		tags_.push_back(*it);
	}
}

void rss_item::set_enclosure_url(const std::string& url) {
	enclosure_url_ = url;
}

void rss_item::set_enclosure_type(const std::string& type) {
	enclosure_type_ = type;
}

std::string rss_item::title() const {
	std::string retval;
	if (title_.length()>0)
		retval = utils::convert_text(title_, nl_langinfo(CODESET), "utf-8");
	return retval;
}

std::string rss_item::author() const {
	return utils::convert_text(author_, nl_langinfo(CODESET), "utf-8");
}

std::string rss_item::description() const {
	return utils::convert_text(description_, nl_langinfo(CODESET), "utf-8");
}

std::string rss_feed::title() const {
	bool found_title = false;
	std::string alt_title;
	for (std::vector<std::string>::const_iterator it=tags_.begin();it!=tags_.end();++it) {
		if (it->substr(0,1) == "~" || it->substr(0,1) == "!") {
			found_title = true;
			alt_title = it->substr(1, it->length()-1);
			break;
		}
	}
	return found_title ? alt_title : utils::convert_text(title_, nl_langinfo(CODESET), "utf-8");
}

std::string rss_feed::description() const {
	return utils::convert_text(description_, nl_langinfo(CODESET), "utf-8");
}

bool rss_feed::hidden() const {
	for (std::vector<std::string>::const_iterator it=tags_.begin();it!=tags_.end();++it) {
		if (it->substr(0,1) == "!") {
			return true;
		}
	}
	return false;
}

std::tr1::shared_ptr<rss_item> rss_feed::get_item_by_guid(const std::string& guid) {
	scope_mutex lock(&item_mutex);
	return get_item_by_guid_unlocked(guid);
}

std::tr1::shared_ptr<rss_item> rss_feed::get_item_by_guid_unlocked(const std::string& guid) {
	std::tr1::unordered_map<std::string, std::tr1::shared_ptr<rss_item> >::const_iterator it;
	if ((it = items_guid_map.find(guid)) != items_guid_map.end()) {
		return it->second;
	}
	LOG(LOG_DEBUG, "rss_feed::get_item_by_guid_unlocked: hit dummy item!");
	// abort();
	return std::tr1::shared_ptr<rss_item>(new rss_item(ch)); // should never happen!
}

bool rss_item::has_attribute(const std::string& attribname) {
	// LOG(LOG_DEBUG, "rss_item::has_attribute(%s) called", attribname.c_str());
	if (attribname == "title" || 
		attribname == "link" || 
		attribname == "author" || 
		attribname == "content" || 
		attribname == "date"  ||
		attribname == "guid" ||
		attribname == "unread" ||
		attribname == "enclosure_url" ||
		attribname == "enclosure_type" ||
		attribname == "flags" ||
		attribname == "age" ||
		attribname == "articleindex")
			return true;

	// if we have a feed, then forward the request
	if (feedptr)
		return feedptr->rss_feed::has_attribute(attribname);

	return false;
}

std::string rss_item::get_attribute(const std::string& attribname) {
	if (attribname == "title")
		return title();
	else if (attribname == "link")
		return link();
	else if (attribname == "author")
		return author();
	else if (attribname == "content")
		return description();
	else if (attribname == "date")
		return pubDate();
	else if (attribname == "guid")
		return guid();
	else if (attribname == "unread")
		return unread_ ? "yes" : "no";
	else if (attribname == "enclosure_url")
		return enclosure_url();
	else if (attribname == "enclosure_type")
		return enclosure_type();
	else if (attribname == "flags")
		return flags();
	else if (attribname == "age")
		return utils::to_s((time(NULL) - pubDate_timestamp()) / 86400);
	else if (attribname == "articleindex")
		return utils::to_s(idx);

	// if we have a feed, then forward the request
	if (feedptr)
		return feedptr->rss_feed::get_attribute(attribname);

	return "";
}

void rss_item::update_flags() {
	if (ch) {
		ch->update_rssitem_flags(this);
	}
}

void rss_item::set_flags(const std::string& ff) {
	oldflags_ = flags_;
	flags_ = ff;
	sort_flags();
}

void rss_item::sort_flags() {
	std::sort(flags_.begin(), flags_.end());

	for (std::string::iterator it=flags_.begin();flags_.size() > 0 && it!=flags_.end();++it) {
		if (!isalpha(*it)) {
			flags_.erase(it);
			it = flags_.begin();
		}
	}

	for (unsigned int i=0;i<flags_.size();++i) {
		if (i < (flags_.size()-1)) {
			if (flags_[i] == flags_[i+1]) {
				flags_.erase(i+1,i+1);
				--i;
			}
		}
	}
}

bool rss_feed::has_attribute(const std::string& attribname) {
	if (attribname == "feedtitle" ||
		attribname == "description" ||
		attribname == "feedlink" ||
		attribname == "feeddate" ||
		attribname == "rssurl" ||
		attribname == "unread_count" ||
		attribname == "total_count" ||
		attribname == "tags" ||
		attribname == "feedindex")
			return true;
	return false;
}

std::string rss_feed::get_attribute(const std::string& attribname) {
	if (attribname == "feedtitle")
		return title();
	else if (attribname == "description")
		return description();
	else if (attribname == "feedlink")
		return title();
	else if (attribname == "feeddate")
		return pubDate();
	else if (attribname == "rssurl")
		return rssurl();
	else if (attribname == "unread_count") {
		return utils::to_s(unread_item_count());
	} else if (attribname == "total_count") {
		return utils::to_s(items_.size());
	} else if (attribname == "tags") {
		return get_tags();
	} else if (attribname == "feedindex") {
		return utils::to_s(idx);
	}
	return "";
}

void rss_ignores::handle_action(const std::string& action, const std::vector<std::string>& params) {
	if (action == "ignore-article") {
		if (params.size() < 2)
			throw confighandlerexception(AHS_TOO_FEW_PARAMS);
		std::string ignore_rssurl = params[0];
		std::string ignore_expr = params[1];
		matcher m;
		if (!m.parse(ignore_expr))
			throw confighandlerexception(utils::strprintf(_("couldn't parse filter expression `%s': %s"), ignore_expr.c_str(), m.get_parse_error().c_str()));
		ignores.push_back(feedurl_expr_pair(ignore_rssurl, new matcher(ignore_expr)));
	} else if (action == "always-download") {
		for (std::vector<std::string>::const_iterator it=params.begin();it!=params.end();++it) {
			ignores_lastmodified.push_back(*it);
		}
	} else if (action == "reset-unread-on-update") {
		for (std::vector<std::string>::const_iterator it=params.begin();it!=params.end();++it) {
			resetflag.push_back(*it);
		}
	} else
		throw confighandlerexception(AHS_INVALID_COMMAND);
}

void rss_ignores::dump_config(std::vector<std::string>& config_output) {
	for (std::vector<feedurl_expr_pair>::iterator it = ignores.begin();it!=ignores.end();++it) {
		std::string configline = "ignore-article ";
		if (it->first == "*")
			configline.append("*");
		else
			configline.append(utils::quote(it->first));
		configline.append(" ");
		configline.append(utils::quote(it->second->get_expression()));
		config_output.push_back(configline);
	}
	for (std::vector<std::string>::iterator it=ignores_lastmodified.begin();it!=ignores_lastmodified.end();++it) {
		config_output.push_back(utils::strprintf("always-download %s", utils::quote(*it).c_str()));
	}
	for (std::vector<std::string>::iterator it=resetflag.begin();it!=resetflag.end();++it) {
		config_output.push_back(utils::strprintf("reset-unread-on-update %s", utils::quote(*it).c_str()));
	}
}

rss_ignores::~rss_ignores() {
	for (std::vector<feedurl_expr_pair>::iterator it=ignores.begin();it!=ignores.end();++it) {
		delete it->second;
	}
}

bool rss_ignores::matches(rss_item* item) {
	for (std::vector<feedurl_expr_pair>::iterator it=ignores.begin();it!=ignores.end();++it) {
		LOG(LOG_DEBUG, "rss_ignores::matches: it->first = `%s' item->feedurl = `%s'", it->first.c_str(), item->feedurl().c_str());
		if (it->first == "*" || item->feedurl() == it->first) {
			if (it->second->matches(item)) {
				LOG(LOG_DEBUG, "rss_ignores::matches: found match");
				return true;
			}
		}
	}
	return false;
}

bool rss_ignores::matches_lastmodified(const std::string& url) {
	for (std::vector<std::string>::iterator it=ignores_lastmodified.begin();it!=ignores_lastmodified.end();++it) {
		if (url == *it)
			return true;
	}
	return false;
}

bool rss_ignores::matches_resetunread(const std::string& url) {
	for (std::vector<std::string>::iterator it=resetflag.begin();it!=resetflag.end();++it) {
		if (url == *it)
			return true;
	}
	return false;
}

void rss_feed::update_items(std::vector<std::tr1::shared_ptr<rss_feed> > feeds) {
	scope_mutex lock(&item_mutex);
	if (query.length() == 0)
		return;

	LOG(LOG_DEBUG, "rss_feed::update_items: query = `%s'", query.c_str());


	struct timeval tv1, tv2, tvx;
	gettimeofday(&tv1, NULL);

	matcher m(query);

	items_.clear();
	items_guid_map.clear();

	for (std::vector<std::tr1::shared_ptr<rss_feed> >::iterator it=feeds.begin();it!=feeds.end();++it) {
		if ((*it)->rssurl().substr(0,6) != "query:") { // don't fetch items from other query feeds!
			for (std::vector<std::tr1::shared_ptr<rss_item> >::iterator jt=(*it)->items().begin();jt!=(*it)->items().end();++jt) {
				if (m.matches(jt->get())) {
					LOG(LOG_DEBUG, "rss_feed::update_items: matcher matches!");
					(*jt)->set_feedptr(*it);
					items_.push_back(*jt);
					items_guid_map[(*jt)->guid()] = *jt;
				}
			}
		}
	}

	gettimeofday(&tvx, NULL);

	std::sort(items_.begin(), items_.end());

	gettimeofday(&tv2, NULL);
	unsigned long diff = (((tv2.tv_sec - tv1.tv_sec) * 1000000) + tv2.tv_usec) - tv1.tv_usec;
	unsigned long diffx = (((tv2.tv_sec - tvx.tv_sec) * 1000000) + tv2.tv_usec) - tvx.tv_usec;
	LOG(LOG_DEBUG, "rss_feed::update_items matching took %lu.%06lu s", diff / 1000000, diff % 1000000);
	LOG(LOG_DEBUG, "rss_feed::update_items sorting took %lu.%06lu s", diffx / 1000000, diffx % 1000000);
}

void rss_feed::set_rssurl(const std::string& u) {
	rssurl_ = u;
	if (rssurl_.substr(0,6) == "query:") {
		std::vector<std::string> tokens = utils::tokenize_quoted(u, ":");
		if (tokens.size() < 3) {
			throw std::string(_("too few arguments"));
		}
		LOG(LOG_DEBUG, "rss_feed::set_rssurl: query name = `%s' expr = `%s'", tokens[1].c_str(), tokens[2].c_str());
		set_title(tokens[1]);
		set_query(tokens[2]);
	}
}

struct sort_item_by_title : public std::binary_function<std::tr1::shared_ptr<rss_item>, std::tr1::shared_ptr<rss_item>, bool> {
	bool reverse;
	sort_item_by_title(bool b) : reverse(b) { }
	bool operator()(std::tr1::shared_ptr<rss_item> a, std::tr1::shared_ptr<rss_item> b) {
		return reverse ?  (strcasecmp(a->title().c_str(), b->title().c_str()) > 0) : (strcasecmp(a->title().c_str(), b->title().c_str()) < 0);
	}
};

struct sort_item_by_flags : public std::binary_function<std::tr1::shared_ptr<rss_item>, std::tr1::shared_ptr<rss_item>, bool> {
	bool reverse;
	sort_item_by_flags(bool b) : reverse(b) { }
	bool operator()(std::tr1::shared_ptr<rss_item> a, std::tr1::shared_ptr<rss_item> b) {
		return reverse ?  (strcmp(a->flags().c_str(), b->flags().c_str()) > 0) : (strcmp(a->flags().c_str(), b->flags().c_str()) < 0);
	}
};

struct sort_item_by_author : public std::binary_function<std::tr1::shared_ptr<rss_item>, std::tr1::shared_ptr<rss_item>, bool> {
	bool reverse;
	sort_item_by_author(bool b) : reverse(b) { }
	bool operator()(std::tr1::shared_ptr<rss_item> a, std::tr1::shared_ptr<rss_item> b) {
		return reverse ?  (strcmp(a->author().c_str(), b->author().c_str()) > 0) : (strcmp(a->author().c_str(), b->author().c_str()) < 0);
	}
};

struct sort_item_by_link : public std::binary_function<std::tr1::shared_ptr<rss_item>, std::tr1::shared_ptr<rss_item>, bool> {
	bool reverse;
	sort_item_by_link(bool b) : reverse(b) { }
	bool operator()(std::tr1::shared_ptr<rss_item> a, std::tr1::shared_ptr<rss_item> b) {
		return reverse ?  (strcmp(a->link().c_str(), b->link().c_str()) >  0) : (strcmp(a->link().c_str(), b->link().c_str()) < 0);
	}
};

struct sort_item_by_guid : public std::binary_function<std::tr1::shared_ptr<rss_item>, std::tr1::shared_ptr<rss_item>, bool> {
	bool reverse;
	sort_item_by_guid(bool b) : reverse(b) { }
	bool operator()(std::tr1::shared_ptr<rss_item> a, std::tr1::shared_ptr<rss_item> b) {
		return reverse ?  (strcmp(a->guid().c_str(), b->guid().c_str()) > 0) : (strcmp(a->guid().c_str(), b->guid().c_str()) < 0);
	}
};

struct sort_item_by_date : public std::binary_function<std::tr1::shared_ptr<rss_item>, std::tr1::shared_ptr<rss_item>, bool> {
	bool reverse;
	sort_item_by_date(bool b) : reverse(b) { }
	bool operator()(std::tr1::shared_ptr<rss_item> a, std::tr1::shared_ptr<rss_item> b) {
		return reverse ?  (a->pubDate_timestamp() > b->pubDate_timestamp()) : (a->pubDate_timestamp() < b->pubDate_timestamp());
	}
};

void rss_feed::sort(const std::string& method) {
	scope_mutex lock(&item_mutex);
	sort_unlocked(method);
}

void rss_feed::sort_unlocked(const std::string& method) {
	std::vector<std::string> methods = utils::tokenize(method,"-");
	bool reverse = false;

	if (!methods.empty() && methods[0] == "date") { // date is descending by default
		if (methods.size() > 1 && methods[1] == "asc") {
			reverse = true;
		}
	} else { // all other sort methods are ascending by default
		if (methods.size() > 1 && methods[1] == "desc") {
			reverse = true;
		}
	}

	if (!methods.empty()) {
		if (methods[0] == "title") {
			std::stable_sort(items_.begin(), items_.end(), sort_item_by_title(reverse));
		} else if (methods[0] == "flags") {
			std::stable_sort(items_.begin(), items_.end(), sort_item_by_flags(reverse));
		} else if (methods[0] == "author") {
			std::stable_sort(items_.begin(), items_.end(), sort_item_by_author(reverse));
		} else if (methods[0] == "link") {
			std::stable_sort(items_.begin(), items_.end(), sort_item_by_link(reverse));
		} else if (methods[0] == "guid") {
			std::stable_sort(items_.begin(), items_.end(), sort_item_by_guid(reverse));
		} else if (methods[0] == "date") {
			std::stable_sort(items_.begin(), items_.end(), sort_item_by_date(reverse));
		}
	}
}

void rss_feed::remove_old_deleted_items() {
	scope_mutex lock(&item_mutex);
	std::vector<std::string> guids;
	for (std::vector<std::tr1::shared_ptr<rss_item> >::iterator it=items_.begin();it!=items_.end();++it) {
		guids.push_back((*it)->guid());
	}
	ch->remove_old_deleted_items(rssurl_, guids);
}

void rss_feed::purge_deleted_items() {
	scope_mutex lock(&item_mutex);
	scope_measure m1("rss_feed::purge_deleted_items");
	std::vector<std::tr1::shared_ptr<rss_item> >::iterator it=items_.begin();
	while (it!=items_.end()) {
		if ((*it)->deleted()) {
			items_guid_map.erase((*it)->guid());
			items_.erase(it);
			it = items_.begin(); // items_ modified -> iterator invalidated
		} else {
			++it;
		}
	}
}

void rss_feed::set_feedptrs(std::tr1::shared_ptr<rss_feed> self) {
	scope_mutex lock(&item_mutex);
	for (std::vector<std::tr1::shared_ptr<rss_item> >::iterator it=items_.begin();it!=items_.end();++it) {
		(*it)->set_feedptr(self);
	}
}

void rss_item::set_feedptr(std::tr1::shared_ptr<rss_feed> ptr) {
	feedptr = ptr;
}

std::string rss_feed::get_status() {
	switch (status_) {
		case SUCCESS: return " ";
		case TO_BE_DOWNLOADED: return "_";
		case DURING_DOWNLOAD: return ".";
		case DL_ERROR: return "x";
		default: return "?";
	}
}

void rss_feed::unload() {
	scope_mutex lock(&item_mutex);
	for (std::vector<std::tr1::shared_ptr<rss_item> >::iterator it=items_.begin();it!=items_.end();++it) {
		(*it)->unload();
	}
}

void rss_feed::load() {
	scope_mutex lock(&item_mutex);
	ch->fetch_descriptions(this);
}


}

#include <controller.h>
#include <itemlist_formaction.h>
#include <view.h>
#include <config.h>
#include <logger.h>
#include <exceptions.h>
#include <utils.h>
#include <formatstring.h>
#include <listformatter.h>

#include <cassert>
#include <sstream>

#include <langinfo.h>

#define FILTER_UNREAD_ITEMS "unread != \"no\""

namespace newsbeuter {

itemlist_formaction::itemlist_formaction(view * vv, std::string formstr)
	: formaction(vv,formstr), apply_filter(false), update_visible_items(true), show_searchresult(false),
		search_dummy_feed(new rss_feed(v->get_ctrl()->get_cache())),
		set_filterpos(false), filterpos(0), rxman(0), old_width(0), old_itempos(-1) {
	assert(true==m.parse(FILTER_UNREAD_ITEMS));
	old_sort_order = "";
}

itemlist_formaction::~itemlist_formaction() { }

void itemlist_formaction::process_operation(operation op, bool automatic, std::vector<std::string> * args) {
	bool quit = false;
	bool hardquit = false;

	/*
	 * most of the operations go like this:
	 *   - extract the current position
	 *   - if an item was selected, then fetch it and do something with it
	 */

	std::string itemposname = f->get("itempos");
	unsigned int itempos = utils::to_u(itemposname);

	switch (op) {
		case OP_OPEN: {
				LOG(LOG_INFO, "itemlist_formaction: opening item at pos `%s'", itemposname.c_str());
				if (itemposname.length() > 0 && visible_items.size() != 0) {
					visible_items[itempos].first->set_unread(false); // set article as read
					v->get_ctrl()->mark_article_read(visible_items[itempos].first->guid(), true);
					old_itempos = itempos;
					v->push_itemview(feed, visible_items[itempos].first->guid(), show_searchresult ? searchphrase : "");
					do_redraw = true;
				} else {
					v->show_error(_("No item selected!")); // should not happen
				}
			}
			break;
		case OP_DELETE: {
				scope_measure m1("OP_DELETE");
				if (itemposname.length() > 0 && visible_items.size() != 0) {
					// mark as read
					v->get_ctrl()->mark_article_read(visible_items[itempos].first->guid(), true);
					visible_items[itempos].first->set_unread(false);
					// mark as deleted
					visible_items[itempos].first->set_deleted(!visible_items[itempos].first->deleted());
					v->get_ctrl()->mark_deleted(visible_items[itempos].first->guid(), visible_items[itempos].first->deleted());
					if (itempos < visible_items.size()-1)
						f->set("itempos", utils::strprintf("%u", itempos + 1));
					do_redraw = true;
				} else {
					v->show_error(_("No item selected!")); // should not happen
				}
			}
			break;
		case OP_PURGE_DELETED: {
				scope_measure m1("OP_PURGE_DELETED");
				feed->purge_deleted_items();
				update_visible_items = true;
				do_redraw = true;
			}
			break;
		case OP_OPENBROWSER_AND_MARK: {
				LOG(LOG_INFO, "itemlist_formaction: opening item at pos `%s'", itemposname.c_str());
				if (itemposname.length() > 0 && visible_items.size() != 0) {
					if (itempos < visible_items.size()) {
						visible_items[itempos].first->set_unread(false);
						v->get_ctrl()->mark_article_read(visible_items[itempos].first->guid(), true);
						v->open_in_browser(visible_items[itempos].first->link());
						if (!v->get_cfg()->get_configvalue_as_bool("openbrowser-and-mark-jumps-to-next-unread")) {
							if (itempos < visible_items.size()-1) {
								f->set("itempos", utils::strprintf("%u", itempos + 1));
							}
						} else {
							process_operation(OP_NEXTUNREAD);
						}
						do_redraw = true;
					}
				} else {
					v->show_error(_("No item selected!")); // should not happen
				}
			}
			break;
		case OP_OPENINBROWSER: {
				LOG(LOG_INFO, "itemlist_formaction: opening item at pos `%s'", itemposname.c_str());
				if (itemposname.length() > 0 && visible_items.size() != 0) {
					if (itempos < visible_items.size()) {
						v->open_in_browser(visible_items[itempos].first->link());
						do_redraw = true;
					}
				} else {
					v->show_error(_("No item selected!")); // should not happen
				}
			}
			break;
		case OP_TOGGLEITEMREAD: {
				LOG(LOG_INFO, "itemlist_formaction: toggling item read at pos `%s'", itemposname.c_str());
				if (itemposname.length() > 0) {
					v->set_status(_("Toggling read flag for article..."));
					try {
						if (automatic && args->size() > 0) {
							if ((*args)[0] == "read") {
								visible_items[itempos].first->set_unread(false);
								v->get_ctrl()->mark_article_read(visible_items[itempos].first->guid(), true);
							} else if ((*args)[0] == "unread") {
								visible_items[itempos].first->set_unread(true);
								v->get_ctrl()->mark_article_read(visible_items[itempos].first->guid(), false);
							}
							v->set_status("");
						} else {
							bool unread = visible_items[itempos].first->unread();
							visible_items[itempos].first->set_unread(!unread);
							v->get_ctrl()->mark_article_read(visible_items[itempos].first->guid(), unread);
							v->set_status("");
						}
					} catch (const dbexception& e) {
						v->set_status(utils::strprintf(_("Error while toggling read flag: %s"), e.what()));
					}
					if (!v->get_cfg()->get_configvalue_as_bool("toogleitemread-jumps-to-next-unread")) {
						if (itempos < visible_items.size()-1)
							f->set("itempos", utils::strprintf("%u", itempos + 1));
					} else {
						process_operation(OP_NEXTUNREAD);
					}
					do_redraw = true;
				}
			}
			break;
		case OP_SHOWURLS:
			if (itemposname.length() > 0 && visible_items.size() != 0) {
				if (itempos < visible_items.size()) {
					std::string urlviewer = v->get_cfg()->get_configvalue("external-url-viewer");
					if (urlviewer == "") {
						std::vector<linkpair> links;
						std::vector<std::string> lines;
						htmlrenderer rnd(80);
						std::string baseurl = visible_items[itempos].first->get_base() != "" ? visible_items[itempos].first->get_base() : visible_items[itempos].first->feedurl();
						rnd.render(visible_items[itempos].first->description(), lines, links, baseurl);
						if (!links.empty()) {
							v->push_urlview(links);
						} else {
							v->show_error(_("URL list empty."));
						}
					} else {
						qna_responses.clear();
						qna_responses.push_back(urlviewer);
						this->finished_qna(OP_PIPE_TO);
					}
				}
			} else {
				v->show_error(_("No item selected!")); // should not happen
			}
			break;
		case OP_BOOKMARK: {
				LOG(LOG_INFO, "itemlist_formaction: bookmarking item at pos `%s'", itemposname.c_str());
				if (itemposname.length() > 0 && visible_items.size() != 0) {
					if (itempos < visible_items.size()) {
						if (automatic) {
							qna_responses.clear();
							qna_responses.push_back(visible_items[itempos].first->title());
							qna_responses.push_back(visible_items[itempos].first->link());
							qna_responses.push_back(args->size() > 0 ? (*args)[0] : "");
							this->finished_qna(OP_INT_BM_END);
						} else {
							this->start_bookmark_qna(visible_items[itempos].first->title(), visible_items[itempos].first->link(), "");
						}
					}
				} else {
					v->show_error(_("No item selected!")); // should not happen
				}
			}
			break;
		case OP_EDITFLAGS: {
				if (itemposname.length() > 0 && visible_items.size() != 0) {
					if (itempos < visible_items.size()) {
						if (automatic) {
							if (args->size() > 0) {
								qna_responses.clear();
								qna_responses.push_back((*args)[0]);
								finished_qna(OP_INT_EDITFLAGS_END);
							}
						} else {
							std::vector<qna_pair> qna;
							qna.push_back(qna_pair(_("Flags: "), visible_items[itempos].first->flags()));
							this->start_qna(qna, OP_INT_EDITFLAGS_END);
						}
					}
				} else {
					v->show_error(_("No item selected!")); // should not happen
				}
			}
			break;
		case OP_SAVE: 
			{
				LOG(LOG_INFO, "itemlist_formaction: saving item at pos `%s'", itemposname.c_str());
				if (itemposname.length() > 0 && visible_items.size() != 0) {
					std::string filename ;
					if (automatic) {
						if (args->size() > 0) {
							filename = (*args)[0];
						}
					} else {
						filename = v->run_filebrowser(v->get_filename_suggestion(visible_items[itempos].first->title()));
					}
					save_article(filename, visible_items[itempos].first);
				} else {
					v->show_error(_("Error: no item selected!"));
				}
			}
			break;
		case OP_HELP:
			v->push_help();
			break;
		case OP_RELOAD:
			if (!show_searchresult) {
				LOG(LOG_INFO, "itemlist_formaction: reloading current feed");
				v->get_ctrl()->reload(pos);
				update_visible_items = true;
				do_redraw = true;
			} else {
				v->show_error(_("Error: you can't reload search results."));
			}
			break;
		case OP_QUIT:
			LOG(LOG_INFO, "itemlist_formaction: quitting");
			v->feedlist_mark_pos_if_visible(pos);
			feed->purge_deleted_items();
			feed->unload();
			quit = true;
			break;
		case OP_HARDQUIT:
			LOG(LOG_INFO, "itemlist_formaction: hard quitting");
			v->feedlist_mark_pos_if_visible(pos);
			feed->purge_deleted_items();
			hardquit = true;
			break;
		case OP_NEXTUNREAD:
			LOG(LOG_INFO, "itemlist_formaction: jumping to next unread item");
			if (!jump_to_next_unread_item(false)) {
				if (!v->get_next_unread(this)) {
					v->show_error(_("No unread items."));
				}
			}
			break;
		case OP_PREVUNREAD:
			LOG(LOG_INFO, "itemlist_formaction: jumping to previous unread item");
			if (!jump_to_previous_unread_item(false)) {
				if (!v->get_previous_unread(this)) {
					v->show_error(_("No unread items."));
				}
			}
			break;
		case OP_NEXT:
			LOG(LOG_INFO, "itemlist_formaction: jumping to next item");
			if (!jump_to_next_item(false)) {
				if (!v->get_next(this)) {
					v->show_error(_("Already on last item."));
				}
			}
			break;
		case OP_PREV:
			LOG(LOG_INFO, "itemlist_formaction: jumping to previous item");
			if (!jump_to_previous_item(false)) {
				if (!v->get_previous(this)) {
					v->show_error(_("Already on first item."));
				}
			}
			break;
		case OP_RANDOMUNREAD:
			if (!jump_to_random_unread_item()) {
				if (!v->get_random_unread(this)) {
					v->show_error(_("No unread items."));
				}
			}
			break;
		case OP_NEXTUNREADFEED:
			if (!v->get_next_unread_feed(this)) {
				v->show_error(_("No unread feeds."));
			}
			break;
		case OP_PREVUNREADFEED:
			if (!v->get_prev_unread_feed(this)) {
				v->show_error(_("No unread feeds."));
			}
			break;
		case OP_NEXTFEED:
			if (!v->get_next_feed(this)) {
				v->show_error(_("Already on last feed."));
			}
			break;
		case OP_PREVFEED:
			if (!v->get_prev_feed(this)) {
				v->show_error(_("Already on first feed."));
			}
			break;
		case OP_MARKFEEDREAD:
			LOG(LOG_INFO, "itemlist_formaction: marking feed read");
			v->set_status(_("Marking feed read..."));
			try {
				if (feed->rssurl() != "") {
					v->get_ctrl()->mark_all_read(pos);
				} else {
					{
						scope_mutex lock(&feed->item_mutex);
						LOG(LOG_DEBUG, "itemlist_formaction: oh, it looks like I'm in a pseudo-feed (search result, query feed)");
						for (std::vector<std::tr1::shared_ptr<rss_item> >::iterator it=feed->items().begin();it!=feed->items().end();++it) {
							(*it)->set_unread_nowrite_notify(false, true); // TODO: do we need to call mark_article_read here, too?
						}
					}
					v->get_ctrl()->catchup_all(feed);
				}
				if (v->get_cfg()->get_configvalue_as_bool("markfeedread-jumps-to-next-unread"))
					process_operation(OP_NEXTUNREAD);
				do_redraw = true;
				v->set_status("");
			} catch (const dbexception& e) {
				v->show_error(utils::strprintf(_("Error: couldn't mark feed read: %s"), e.what()));
			}
			break;
		case OP_TOGGLESHOWREAD:
			m.parse(FILTER_UNREAD_ITEMS);
			LOG(LOG_DEBUG, "itemlist_formaction: toggling show-read-articles");
			if (v->get_cfg()->get_configvalue_as_bool("show-read-articles")) {
				v->get_cfg()->set_configvalue("show-read-articles", "no");
				apply_filter = true;
			} else {
				v->get_cfg()->set_configvalue("show-read-articles", "yes");
				apply_filter = false;
			}
			save_filterpos();
			update_visible_items = true;
			do_redraw = true;
			break;
		case OP_PIPE_TO:
			if(visible_items.size() != 0) {
				std::vector<qna_pair> qna;
				if (automatic) {
					if (args->size() > 0) {
						qna_responses.clear();
						qna_responses.push_back((*args)[0]);
						finished_qna(OP_PIPE_TO);
					}
				} else {
					qna.push_back(qna_pair(_("Pipe article to command: "), ""));
					this->start_qna(qna, OP_PIPE_TO, &cmdlinehistory);
				}
			} else {
				v->show_error(_("No item selected!"));
			}
			break;
		case OP_SEARCH: {
				std::vector<qna_pair> qna;
				if (automatic) {
					if (args->size() > 0) {
						qna_responses.clear();
						qna_responses.push_back((*args)[0]);
						finished_qna(OP_INT_START_SEARCH);
					}
				} else {
					qna.push_back(qna_pair(_("Search for: "), ""));
					this->start_qna(qna, OP_INT_START_SEARCH, &searchhistory);
				}
			}
			break;
		case OP_EDIT_URLS:
			v->get_ctrl()->edit_urls_file();
			break;
		case OP_SELECTFILTER:
			if (v->get_ctrl()->get_filters().size() > 0) {
				std::string newfilter;
				if (automatic) {
					if (args->size() > 0)
						newfilter = (*args)[0];
				} else {
					newfilter = v->select_filter(v->get_ctrl()->get_filters().get_filters());
					LOG(LOG_DEBUG,"itemlist_formaction::run: newfilters = %s", newfilter.c_str());
				}
				if (newfilter != "") {
					filterhistory.add_line(newfilter);
					if (newfilter.length() > 0) {
						if (!m.parse(newfilter)) {
							v->show_error(utils::strprintf(_("Error: couldn't parse filter command `%s': %s"), newfilter.c_str(), m.get_parse_error().c_str()));
							m.parse(FILTER_UNREAD_ITEMS);
						} else {
							apply_filter = true;
							update_visible_items = true;
							do_redraw = true;
							save_filterpos();
						}
					}
				}
			} else {
				v->show_error(_("No filters defined."));
			}

			break;
		case OP_SETFILTER: 
			if (automatic) {
				if (args->size() > 0) {
					qna_responses.clear();
					qna_responses.push_back((*args)[0]);
					this->finished_qna(OP_INT_END_SETFILTER);
				}
			} else {
				std::vector<qna_pair> qna;
				qna.push_back(qna_pair(_("Filter: "), ""));
				this->start_qna(qna, OP_INT_END_SETFILTER, &filterhistory);
			}
			break;
		case OP_CLEARFILTER:
			apply_filter = false;
			update_visible_items = true;
			do_redraw = true;
			save_filterpos();
			break;
		case OP_SORT: {
				char c = v->confirm(_("Sort by (d)ate/(t)itle/(f)lags/(a)uthor/(l)ink/(g)uid?"), _("dtfalg"));
				if (!c) break;
				std::string result(1, c);
				if (result == _("d")) {
					v->get_cfg()->set_configvalue("article-sort-order", "date-asc");
				} else if (result == _("t")) {
					v->get_cfg()->set_configvalue("article-sort-order", "title-asc");
				} else if (result == _("f")) {
					v->get_cfg()->set_configvalue("article-sort-order", "flags-asc");
				} else if (result == _("a")) {
					v->get_cfg()->set_configvalue("article-sort-order", "author-asc");
				} else if (result == _("l")) {
					v->get_cfg()->set_configvalue("article-sort-order", "link-asc");
				} else if (result == _("g")) {
					v->get_cfg()->set_configvalue("article-sort-order", "guid-asc");
				}
			}
			break;
		case OP_REVSORT: {
				char c = v->confirm(_("Reverse Sort by (d)ate/(t)itle/(f)lags/(a)uthor/(l)ink/(g)uid?"), _("dtfalg"));
				if (!c) break;
				std::string result(1, c);
				if (result == _("d")) {
					v->get_cfg()->set_configvalue("article-sort-order", "date-desc");
				} else if (result == _("t")) {
					v->get_cfg()->set_configvalue("article-sort-order", "title-desc");
				} else if (result == _("f")) {
					v->get_cfg()->set_configvalue("article-sort-order", "flags-desc");
				} else if (result == _("a")) {
					v->get_cfg()->set_configvalue("article-sort-order", "author-desc");
				} else if (result == _("l")) {
					v->get_cfg()->set_configvalue("article-sort-order", "link-desc");
				} else if (result == _("g")) {
					v->get_cfg()->set_configvalue("article-sort-order", "guid-desc");
				}
			}
			break;
		case OP_INT_RESIZE:
			do_redraw = true;
			break;
		default:
			break;
	}
	if (hardquit) {
		while (v->formaction_stack_size() > 0) {
			v->pop_current_formaction();
		}
	} else if (quit) {
		v->pop_current_formaction();
	}
}

void itemlist_formaction::finished_qna(operation op) {
	formaction::finished_qna(op); // important!

	switch (op) {
		case OP_INT_END_SETFILTER:
			qna_end_setfilter();
			break;

		case OP_INT_EDITFLAGS_END:
			qna_end_editflags();
			break;

		case OP_INT_START_SEARCH:
			qna_start_search();
			break;

		case OP_PIPE_TO: {
				std::string itemposname = f->get("itempos");
				unsigned int itempos = utils::to_u(itemposname);
				if (itemposname.length() > 0) {
					std::string cmd = qna_responses[0];
					std::ostringstream ostr;
					v->get_ctrl()->write_item(visible_items[itempos].first, ostr);
					v->push_empty_formaction();
					stfl::reset();
					FILE * f = popen(cmd.c_str(), "w");
					if (f) {
						std::string data = ostr.str();
						fwrite(data.c_str(), data.length(), 1, f);
						pclose(f);
					}
					v->pop_current_formaction();
				}
			}
			break;

		default:
			break;
	}
}

void itemlist_formaction::qna_end_setfilter() {
	std::string filtertext = qna_responses[0];
	filterhistory.add_line(filtertext);

	if (filtertext.length() > 0) {

		if (!m.parse(filtertext)) {
			v->show_error(_("Error: couldn't parse filter command!"));
			return;
		}

		apply_filter = true;
		update_visible_items = true;
		do_redraw = true;
		save_filterpos();
	}
}

void itemlist_formaction::qna_end_editflags() {
	std::string itemposname = f->get("itempos");
	if (itemposname.length() == 0) {
		v->show_error(_("No item selected!")); // should not happen
		return;
	}

	std::istringstream posname(itemposname);
	unsigned int itempos = 0;
	posname >> itempos;
	if (itempos < visible_items.size()) {
		visible_items[itempos].first->set_flags(qna_responses[0]);
		v->get_ctrl()->update_flags(visible_items[itempos].first);
		v->set_status(_("Flags updated."));
		LOG(LOG_DEBUG, "itemlist_formaction::finished_qna: updated flags");
		do_redraw = true;
	}
}

void itemlist_formaction::qna_start_search() {
	searchphrase = qna_responses[0];
	if (searchphrase.length() == 0)
		return;

	v->set_status(_("Searching..."));
	searchhistory.add_line(searchphrase);
	std::vector<std::tr1::shared_ptr<rss_item> > items;
	try {
		std::string utf8searchphrase = utils::convert_text(searchphrase, "utf-8", nl_langinfo(CODESET));
		if (show_searchresult) {
			items = v->get_ctrl()->search_for_items(utf8searchphrase, "");
		} else {
			items = v->get_ctrl()->search_for_items(utf8searchphrase, feed->rssurl());
		}
	} catch (const dbexception& e) {
		v->show_error(utils::strprintf(_("Error while searching for `%s': %s"), searchphrase.c_str(), e.what()));
		return;
	}

	if (items.empty()) {
		v->show_error(_("No results."));
		return;
	}

	{
		scope_mutex lock(&search_dummy_feed->item_mutex);
		for (std::vector<std::tr1::shared_ptr<rss_item> >::iterator it=items.begin();it!=items.end();++it) {
			search_dummy_feed->add_item(*it);
		}
	}

	if (show_searchresult) {
		v->pop_current_formaction();
	}
	v->push_searchresult(search_dummy_feed, searchphrase);
}

void itemlist_formaction::do_update_visible_items() {
	if (!update_visible_items)
		return;

	update_visible_items = false;

	scope_mutex lock(&feed->item_mutex);
	std::vector<std::tr1::shared_ptr<rss_item> >& items = feed->items();

	std::vector<itemptr_pos_pair> new_visible_items;

	/*
	 * this method doesn't redraw, all it does is to go through all
	 * items of a feed, and fill the visible_items vector by checking
	 * (if applicable) whether an items matches the currently active filter.
	 */

	unsigned int i=0;
	for (std::vector<std::tr1::shared_ptr<rss_item> >::iterator it = items.begin(); it != items.end(); ++it, ++i) {
		(*it)->set_index(i+1);
		if (!apply_filter || m.matches(it->get())) {
			new_visible_items.push_back(itemptr_pos_pair(*it, i));
		}
	}

	LOG(LOG_DEBUG, "itemlist_formaction::do_update_visible_items: size = %u", visible_items.size());

	visible_items = new_visible_items;

	do_redraw = true;
}

void itemlist_formaction::prepare() {
	scope_mutex mtx(&redraw_mtx);

	std::string sort_order = v->get_cfg()->get_configvalue("article-sort-order");
	if (sort_order != old_sort_order) {
		feed->sort(sort_order);
		old_sort_order = sort_order;
		update_visible_items = true;
		do_redraw = true;
	}

	try {
		do_update_visible_items();
	} catch (matcherexception& e) {
		v->show_error(utils::strprintf(_("Error: applying the filter failed: %s"), e.what()));
		return;
	}

	if (v->get_cfg()->get_configvalue_as_bool("mark-as-read-on-hover")) {
		std::string itemposname = f->get("itempos");
		if (itemposname.length() > 0) {
			unsigned int itempos = utils::to_u(itemposname);
			if (visible_items[itempos].first->unread()) {
				visible_items[itempos].first->set_unread(false);
				v->get_ctrl()->mark_article_read(visible_items[itempos].first->guid(), true);
				do_redraw = true;
			}
		}
	}

	unsigned int width = utils::to_u(f->get("items:w"));

	if (old_width != width) {
		do_redraw = true;
		old_width = width;
	}

	if (!do_redraw)
		return;
	do_redraw = false;

	listformatter listfmt;

	std::string datetimeformat = v->get_cfg()->get_configvalue("datetime-format");
	std::string itemlist_format = v->get_cfg()->get_configvalue("articlelist-format");


	for (std::vector<itemptr_pos_pair>::iterator it = visible_items.begin(); it != visible_items.end(); ++it) {
		std::string tmp_itemlist_format = itemlist_format;
		fmtstr_formatter fmt;

		fmt.register_fmt('i', utils::strprintf("%u",it->second + 1));
		fmt.register_fmt('f', gen_flags(it->first));
		fmt.register_fmt('D', gen_datestr(it->first->pubDate_timestamp(), datetimeformat.c_str()));
		if (feed->rssurl() != it->first->feedurl() && it->first->get_feedptr() != NULL) {
			fmt.register_fmt('T', utils::replace_all(it->first->get_feedptr()->title(), "<", "<>"));
		}
		fmt.register_fmt('t', utils::replace_all(it->first->title(), "<", "<>"));
		fmt.register_fmt('a', utils::replace_all(it->first->author(), "<", "<>"));
		fmt.register_fmt('L', it->first->length());

		if (rxman) {
			int id;
			if ((id = rxman->article_matches(it->first.get())) != -1) {
				tmp_itemlist_format = utils::strprintf("<%d>%s</>", id, itemlist_format.c_str());
			}
		}

		listfmt.add_line(fmt.do_format(tmp_itemlist_format, width), it->second);
	}

	f->modify("items","replace_inner", listfmt.format_list(rxman, "articlelist"));
	
	set_head(feed->title(),feed->unread_item_count(),feed->items().size(), feed->rssurl());

	prepare_set_filterpos();
}

void itemlist_formaction::init() {
	f->set("itempos","0");
	f->set("msg","");
	do_redraw = true;
	set_keymap_hints();
	apply_filter = !(v->get_cfg()->get_configvalue_as_bool("show-read-articles"));
	update_visible_items = true;
	do_update_visible_items();
	if (v->get_cfg()->get_configvalue_as_bool("goto-first-unread")) {
		jump_to_next_unread_item(true);
	}
	f->run(-3); // FRUN - compute all widget dimensions
}

void itemlist_formaction::set_head(const std::string& s, unsigned int unread, unsigned int total, const std::string &url) {
	/*
	 * Since the itemlist_formaction is also used to display search results, we always need to set the right title
	 */
	std::string title;
	fmtstr_formatter fmt;

	std::string listwidth = f->get("items:w");
	std::istringstream is(listwidth);
	unsigned int width;
	is >> width;

	fmt.register_fmt('N', PROGRAM_NAME);
	fmt.register_fmt('V', PROGRAM_VERSION);

	fmt.register_fmt('u', utils::to_s(unread));
	fmt.register_fmt('t', utils::to_s(total));

	fmt.register_fmt('T', s);
	fmt.register_fmt('U', utils::censor_url(url));

	if (!show_searchresult) {
		title = fmt.do_format(v->get_cfg()->get_configvalue("articlelist-title-format"));
	} else {
		title = fmt.do_format(v->get_cfg()->get_configvalue("searchresult-title-format"));
	}
	f->set("head", title);
}

bool itemlist_formaction::jump_to_previous_unread_item(bool start_with_last) {
	int itempos;
	std::istringstream is(f->get("itempos"));
	is >> itempos;
	for (int i=(start_with_last?itempos:(itempos-1));i>=0;--i) {
		LOG(LOG_DEBUG, "itemlist_formaction::jump_to_previous_unread_item: visible_items[%u] unread = %s", i, visible_items[i].first->unread() ? "true" : "false");
		if (visible_items[i].first->unread()) {
			f->set("itempos", utils::to_s(i));
			return true;
		}
	}
	for (int i=visible_items.size()-1;i>=itempos;--i) {
		if (visible_items[i].first->unread()) {
			f->set("itempos", utils::to_s(i));
			return true;
		}
	}
	return false;

}

bool itemlist_formaction::jump_to_random_unread_item() {
	bool has_unread_available = false;
	for (unsigned int i=0;i<visible_items.size();++i) {
		if (visible_items[i].first->unread()) {
			has_unread_available = true;
			break;
		}
	}
	if (has_unread_available) {
		for (;;) {
			unsigned int pos = utils::get_random_value(visible_items.size());
			if (visible_items[pos].first->unread()) {
				f->set("itempos", utils::to_s(pos));
				break;
			}
		}
	}
	return has_unread_available;
}

bool itemlist_formaction::jump_to_next_unread_item(bool start_with_first) {
	unsigned int itempos;
	std::istringstream is(f->get("itempos"));
	is >> itempos;
	LOG(LOG_DEBUG, "itemlist_formaction::jump_to_next_unread_item: itempos = %u visible_items.size = %u", itempos, visible_items.size());
	for (unsigned int i=(start_with_first?itempos:(itempos+1));i<visible_items.size();++i) {
		LOG(LOG_DEBUG, "itemlist_formaction::jump_to_next_unread_item: i = %u", i);
		if (visible_items[i].first->unread()) {
			f->set("itempos", utils::to_s(i));
			return true;
		}
	}
	for (unsigned int i=0;i<=itempos;++i) {
		LOG(LOG_DEBUG, "itemlist_formaction::jump_to_next_unread_item: i = %u", i);
		if (visible_items[i].first->unread()) {
			f->set("itempos", utils::to_s(i));
			return true;
		}
	}
	return false;
}

bool itemlist_formaction::jump_to_previous_item(bool start_with_last) {
	int itempos;
	std::istringstream is(f->get("itempos"));
	is >> itempos;
	for (int i=(start_with_last?itempos:(itempos-1));i>=0;--i) {
		LOG(LOG_DEBUG, "itemlist_formaction::jump_to_previous_item: visible_items[%u]", i);
		f->set("itempos", utils::to_s(i));
		return true;
	}
	return false; // not sure if we should exit here or continue
	// wrap to last item
	for (int i=visible_items.size()-1;i>=itempos;--i) {
		f->set("itempos", utils::to_s(i));
		return true;
	}
	return false;

}

bool itemlist_formaction::jump_to_next_item(bool start_with_first) {
	unsigned int itempos;
	std::istringstream is(f->get("itempos"));
	is >> itempos;
	LOG(LOG_DEBUG, "itemlist_formaction::jump_to_next_item: itempos = %u visible_items.size = %u", itempos, visible_items.size());
	for (unsigned int i=(start_with_first?itempos:(itempos+1));i<visible_items.size();++i) {
		LOG(LOG_DEBUG, "itemlist_formaction::jump_to_next_item: i = %u", i);
		f->set("itempos", utils::to_s(i));
		return true;
	}
	return false; // not sure if we should exit here or continue
	// wrap to first item
	for (unsigned int i=0;i<=itempos;++i) {
		LOG(LOG_DEBUG, "itemlist_formaction::jump_to_next_item: i = %u", i);
		f->set("itempos", utils::to_s(i));
		return true;
	}
	return false;
}

std::string itemlist_formaction::get_guid() {
	unsigned int itempos;
	std::istringstream is(f->get("itempos"));
	is >> itempos;
	return visible_items[itempos].first->guid();
}

keymap_hint_entry * itemlist_formaction::get_keymap_hint() {
	static keymap_hint_entry hints[] = {
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
	return hints;
}

void itemlist_formaction::handle_cmdline_num(unsigned int idx) {
	if (idx > 0 && idx <= visible_items[visible_items.size()-1].second + 1) {
		int i = get_pos(idx - 1);
		if (i == -1) {
			v->show_error(_("Position not visible!"));
		} else {
			f->set("itempos", utils::to_s(i));
		}
	} else {
		v->show_error(_("Invalid position!"));
	}
}

void itemlist_formaction::handle_cmdline(const std::string& cmd) {
	unsigned int idx = 0;
	if (1==sscanf(cmd.c_str(),"%u",&idx)) {
		handle_cmdline_num(idx);
	} else {
		std::vector<std::string> tokens = utils::tokenize_quoted(cmd);
		if (tokens.empty())
			return;
		if (tokens[0] == "save" && tokens.size() >= 2) {
			std::string filename = utils::resolve_tilde(tokens[1]);
			std::string itemposname = f->get("itempos");
			LOG(LOG_INFO, "itemlist_formaction::handle_cmdline: saving item at pos `%s' to `%s'", itemposname.c_str(), filename.c_str());
			if (itemposname.length() > 0) {
				std::istringstream posname(itemposname);
				unsigned int itempos = 0;
				posname >> itempos;

				save_article(filename, visible_items[itempos].first);
			} else {
				v->show_error(_("Error: no item selected!"));
			}
		} else {
			formaction::handle_cmdline(cmd);
		}
	}
}

int itemlist_formaction::get_pos(unsigned int realidx) {
	for (unsigned int i=0;i<visible_items.size();++i) {
		if (visible_items[i].second == realidx)
			return i;
	}
	return -1;
}

void itemlist_formaction::recalculate_form() {
	formaction::recalculate_form();
	set_update_visible_items(true);

	std::string itemposname = f->get("itempos");
	unsigned int itempos = utils::to_u(itemposname);
	
	// If the old position was set and it is less than the itempos, use it for the feed's itempos
	// Correct the problem when you open itemview and jump to next then exit to itemlist and the itempos is wrong
	// This only applies when "show-read-articles" is set to false
	if ( (old_itempos != -1) && itempos > (unsigned int)old_itempos
		&& ! v->get_cfg()->get_configvalue_as_bool("show-read-articles") )
	{
		f->set("itempos", utils::strprintf("%u", old_itempos));
		old_itempos = -1; // Reset
	}
}

void itemlist_formaction::save_article(const std::string& filename, std::tr1::shared_ptr<rss_item> item) {
	if (filename == "") {
		v->show_error(_("Aborted saving."));
	} else {
		try {
			v->get_ctrl()->write_item(item, filename);
			v->show_error(utils::strprintf(_("Saved article to %s"), filename.c_str()));
		} catch (...) {
			v->show_error(utils::strprintf(_("Error: couldn't save article to %s"), filename.c_str()));
		}
	}
}

void itemlist_formaction::save_filterpos() {
	std::istringstream is(f->get("itempos"));
	unsigned int i;
	is >> i;
	if (i<visible_items.size()) {
		filterpos = visible_items[i].second;
		set_filterpos = true;
	}
}

void itemlist_formaction::set_regexmanager(regexmanager * r) {
	rxman = r;
	std::vector<std::string>& attrs = r->get_attrs("articlelist");
	unsigned int i=0;
	std::string attrstr;
	for (std::vector<std::string>::iterator it=attrs.begin();it!=attrs.end();++it,++i) {
		attrstr.append(utils::strprintf("@style_%u_normal:%s ", i, it->c_str()));
		attrstr.append(utils::strprintf("@style_%u_focus:%s ", i, it->c_str()));
	}
	std::string textview = utils::strprintf("{list[items] .expand:vh style_normal[listnormal]: style_focus[listfocus]:fg=yellow,bg=blue,attr=bold pos_name[itemposname]: pos[itempos]:0 %s richtext:1}", attrstr.c_str());
	f->modify("items", "replace", textview);
}

std::string itemlist_formaction::gen_flags(std::tr1::shared_ptr<rss_item> item) {
	std::string flags;
	if (item->deleted()) {
		flags.append("D");
	} else if (item->unread()) {
		flags.append("N");
	} else {
		flags.append(" ");
	}
	if (item->flags().length() > 0) {
		flags.append("!");
	} else {
		flags.append(" ");
	}
	return flags;
}

std::string itemlist_formaction::gen_datestr(time_t t, const char * datetimeformat) {
	char datebuf[64];
	struct tm * stm = localtime(&t);
	strftime(datebuf,sizeof(datebuf), datetimeformat, stm);
	return datebuf;
}

void itemlist_formaction::prepare_set_filterpos() {
	if (set_filterpos) {
		set_filterpos = false;
		unsigned int i=0;
		for (std::vector<itemptr_pos_pair>::iterator it=visible_items.begin();it!=visible_items.end();++it, ++i) {
			if (it->second == filterpos) {
				f->set("itempos", utils::to_s(i));
				return;
			}
		}
		f->set("itempos", "0");
	}
}

void itemlist_formaction::set_feed(std::tr1::shared_ptr<rss_feed> fd) {
	LOG(LOG_DEBUG, "itemlist_formaction::set_feed: fd pointer = %p title = `%s'", fd.get(), fd->title().c_str());
	feed = fd;
	feed->load();
	update_visible_items = true; 
	do_update_visible_items();
}

std::string itemlist_formaction::title() {
	if (feed->rssurl() == "") {
		return utils::strprintf(_("Search Result - '%s'"), searchphrase.c_str());
	} else {
		if (feed->rssurl().substr(0,6) == "query:")
			return utils::strprintf(_("Query Feed - %s"), feed->rssurl().substr(6,feed->rssurl().length()-6).c_str());
		else
			return utils::strprintf(_("Article List - %s"), feed->title().c_str());
	}
}

}

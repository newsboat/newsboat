#include <itemlistformaction.h>

#include <cassert>
#include <cstdio>
#include <langinfo.h>
#include <sstream>

#include "config.h"
#include "controller.h"
#include "exceptions.h"
#include "formatstring.h"
#include "logger.h"
#include "strprintf.h"
#include "utils.h"
#include "view.h"

#define FILTER_UNREAD_ITEMS "unread != \"no\""

namespace newsboat {

itemlist_formaction::itemlist_formaction(view* vv, std::string formstr)
	: list_formaction(vv, formstr)
	, pos(0)
	, apply_filter(false)
	, show_searchresult(false)
	, search_dummy_feed(new rss_feed(v->get_ctrl()->get_cache()))
	, set_filterpos(false)
	, filterpos(0)
	, rxman(0)
	, old_width(0)
	, old_itempos(-1)
	, old_sort_strategy({art_sort_method_t::TITLE, sort_direction_t::DESC})
	, invalidated(false)
	, invalidation_mode(InvalidationMode::COMPLETE)
{
	assert(true == m.parse(FILTER_UNREAD_ITEMS));
}

itemlist_formaction::~itemlist_formaction() {}

void itemlist_formaction::process_operation(operation op,
	bool automatic,
	std::vector<std::string>* args)
{
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
		LOG(level::INFO,
			"itemlist_formaction: opening item at pos `%s'",
			itemposname);
		if (itemposname.length() > 0 && visible_items.size() != 0) {
			// no need to mark item as read, the itemview already do
			// that
			old_itempos = itempos;
			v->push_itemview(feed,
				visible_items[itempos].first->guid(),
				show_searchresult ? searchphrase : "");
			invalidate(itempos);
		} else {
			v->show_error(
				_("No item selected!")); // should not happen
		}
	} break;
	case OP_DELETE: {
		scope_measure m1("OP_DELETE");
		if (itemposname.length() > 0 && visible_items.size() != 0) {
			// mark as read
			v->get_ctrl()->mark_article_read(
				visible_items[itempos].first->guid(), true);
			visible_items[itempos].first->set_unread(false);
			// mark as deleted
			visible_items[itempos].first->set_deleted(
				!visible_items[itempos].first->deleted());
			v->get_ctrl()->get_cache()->mark_item_deleted(
				visible_items[itempos].first->guid(),
				visible_items[itempos].first->deleted());
			if (itempos < visible_items.size() - 1)
				f->set("itempos",
					strprintf::fmt("%u", itempos + 1));
			invalidate(itempos);
		} else {
			v->show_error(
				_("No item selected!")); // should not happen
		}
	} break;
	case OP_DELETE_ALL: {
		scope_measure m1("OP_DELETE_ALL");
		if (visible_items.size() > 0) {
			v->get_ctrl()->mark_all_read(pos);
			for (const auto& pair : visible_items) {
				pair.first->set_deleted(true);
			}
			v->get_ctrl()->get_cache()->mark_feed_items_deleted(
				feed->rssurl());
			invalidate(InvalidationMode::COMPLETE);
		}
	} break;
	case OP_PURGE_DELETED: {
		scope_measure m1("OP_PURGE_DELETED");
		feed->purge_deleted_items();
		invalidate(InvalidationMode::COMPLETE);
	} break;
	case OP_OPENBROWSER_AND_MARK: {
		LOG(level::INFO,
			"itemlist_formaction: opening item at pos `%s'",
			itemposname);
		if (itemposname.length() > 0 && visible_items.size() != 0) {
			if (itempos < visible_items.size()) {
				visible_items[itempos].first->set_unread(false);
				v->get_ctrl()->mark_article_read(
					visible_items[itempos].first->guid(),
					true);
				v->open_in_browser(
					visible_items[itempos].first->link());
				if (!v->get_cfg()->get_configvalue_as_bool(
					    "openbrowser-and-mark-jumps-to-"
					    "next-unread")) {
					if (itempos <
						visible_items.size() - 1) {
						f->set("itempos",
							strprintf::fmt("%u",
								itempos + 1));
					}
				} else {
					process_operation(OP_NEXTUNREAD);
				}
				invalidate(itempos);
			}
		} else {
			v->show_error(
				_("No item selected!")); // should not happen
		}
	} break;
	case OP_OPENINBROWSER: {
		LOG(level::INFO,
			"itemlist_formaction: opening item at pos `%s'",
			itemposname);
		if (itemposname.length() > 0 && visible_items.size() != 0) {
			if (itempos < visible_items.size()) {
				v->open_in_browser(
					visible_items[itempos].first->link());
				invalidate(itempos);
			}
		} else {
			v->show_error(
				_("No item selected!")); // should not happen
		}
	} break;
	case OP_OPENALLUNREADINBROWSER: {
		if (feed) {
			LOG(level::INFO,
				"itemlist_formaction: opening all unread items "
				"in "
				"browser");
			open_unread_items_in_browser(feed, false);
		}
	} break;
	case OP_OPENALLUNREADINBROWSER_AND_MARK: {
		if (feed) {
			LOG(level::INFO,
				"itemlist_formaction: opening all unread items "
				"in "
				"browser and marking read");
			open_unread_items_in_browser(feed, true);
			invalidate(InvalidationMode::COMPLETE);
		}
	} break;
	case OP_TOGGLEITEMREAD: {
		LOG(level::INFO,
			"itemlist_formaction: toggling item read at pos `%s'",
			itemposname);
		if (itemposname.length() > 0) {
			v->set_status(_("Toggling read flag for article..."));
			try {
				if (automatic && args->size() > 0) {
					if ((*args)[0] == "read") {
						visible_items[itempos]
							.first->set_unread(
								false);
						v->get_ctrl()->mark_article_read(
							visible_items[itempos]
								.first->guid(),
							true);
					} else if ((*args)[0] == "unread") {
						visible_items[itempos]
							.first->set_unread(
								true);
						v->get_ctrl()->mark_article_read(
							visible_items[itempos]
								.first->guid(),
							false);
					}
					v->set_status("");
				} else {
					// mark as undeleted
					visible_items[itempos]
						.first->set_deleted(false);
					v->get_ctrl()
						->get_cache()
						->mark_item_deleted(
							visible_items[itempos]
								.first->guid(),
							false);
					// toggle read
					bool unread = visible_items[itempos]
							      .first->unread();
					visible_items[itempos]
						.first->set_unread(!unread);
					v->get_ctrl()->mark_article_read(
						visible_items[itempos]
							.first->guid(),
						unread);
					v->set_status("");
				}
			} catch (const dbexception& e) {
				v->set_status(strprintf::fmt(
					_("Error while toggling read flag: %s"),
					e.what()));
			}
			if (!v->get_cfg()->get_configvalue_as_bool(
				    "toggleitemread-jumps-to-next-unread")) {
				if (itempos < visible_items.size() - 1)
					f->set("itempos",
						strprintf::fmt(
							"%u", itempos + 1));
			} else {
				process_operation(OP_NEXTUNREAD);
			}
			invalidate(itempos);
		}
	} break;
	case OP_SHOWURLS:
		if (itemposname.length() > 0 && visible_items.size() != 0) {
			if (itempos < visible_items.size()) {
				std::string urlviewer =
					v->get_cfg()->get_configvalue(
						"external-url-viewer");
				if (urlviewer == "") {
					std::vector<linkpair> links;
					std::vector<std::pair<LineType,
						std::string>>
						lines;
					htmlrenderer rnd;
					std::string baseurl =
						visible_items[itempos]
								.first
								->get_base() !=
							""
						? visible_items[itempos]
							  .first->get_base()
						: visible_items[itempos]
							  .first->feedurl();
					rnd.render(
						visible_items[itempos]
							.first->description(),
						lines,
						links,
						baseurl);
					if (!links.empty()) {
						v->push_urlview(links, feed);
					} else {
						v->show_error(
							_("URL list empty."));
					}
				} else {
					qna_responses.clear();
					qna_responses.push_back(urlviewer);
					this->finished_qna(OP_PIPE_TO);
				}
			}
		} else {
			v->show_error(
				_("No item selected!")); // should not happen
		}
		break;
	case OP_BOOKMARK: {
		LOG(level::INFO,
			"itemlist_formaction: bookmarking item at pos `%s'",
			itemposname);
		if (itemposname.length() > 0 && visible_items.size() != 0) {
			if (itempos < visible_items.size()) {
				if (automatic) {
					qna_responses.clear();
					qna_responses.push_back(
						visible_items[itempos]
							.first->link());
					qna_responses.push_back(
						visible_items[itempos]
							.first->title());
					qna_responses.push_back(args->size() > 0
							? (*args)[0]
							: "");
					qna_responses.push_back(feed->title());
					this->finished_qna(OP_INT_BM_END);
				} else {
					this->start_bookmark_qna(
						visible_items[itempos]
							.first->title(),
						visible_items[itempos]
							.first->link(),
						"",
						feed->title());
				}
			}
		} else {
			v->show_error(
				_("No item selected!")); // should not happen
		}
	} break;
	case OP_EDITFLAGS: {
		if (itemposname.length() > 0 && visible_items.size() != 0) {
			if (itempos < visible_items.size()) {
				if (automatic) {
					if (args->size() > 0) {
						qna_responses.clear();
						qna_responses.push_back(
							(*args)[0]);
						finished_qna(
							OP_INT_EDITFLAGS_END);
					}
				} else {
					std::vector<qna_pair> qna;
					qna.push_back(qna_pair(_("Flags: "),
						visible_items[itempos]
							.first->flags()));
					this->start_qna(
						qna, OP_INT_EDITFLAGS_END);
				}
			}
		} else {
			v->show_error(
				_("No item selected!")); // should not happen
		}
	} break;
	case OP_SAVE: {
		LOG(level::INFO,
			"itemlist_formaction: saving item at pos `%s'",
			itemposname);
		if (itemposname.length() > 0 && visible_items.size() != 0) {
			std::string filename;
			if (automatic) {
				if (args->size() > 0) {
					filename = (*args)[0];
				}
			} else {
				filename = v->run_filebrowser(
					v->get_filename_suggestion(
						visible_items[itempos]
							.first->title()));
			}
			save_article(filename, visible_items[itempos].first);
		} else {
			v->show_error(_("Error: no item selected!"));
		}
	} break;
	case OP_HELP:
		v->push_help();
		break;
	case OP_RELOAD:
		if (!show_searchresult) {
			LOG(level::INFO,
				"itemlist_formaction: reloading current feed");
			v->get_ctrl()->get_reloader()->reload(pos);
			invalidate(InvalidationMode::COMPLETE);
		} else {
			v->show_error(
				_("Error: you can't reload search results."));
		}
		break;
	case OP_QUIT:
		LOG(level::INFO, "itemlist_formaction: quitting");
		v->feedlist_mark_pos_if_visible(pos);
		feed->purge_deleted_items();
		feed->unload();
		quit = true;
		break;
	case OP_HARDQUIT:
		LOG(level::INFO, "itemlist_formaction: hard quitting");
		v->feedlist_mark_pos_if_visible(pos);
		feed->purge_deleted_items();
		hardquit = true;
		break;
	case OP_NEXTUNREAD:
		LOG(level::INFO,
			"itemlist_formaction: jumping to next unread item");
		if (!jump_to_next_unread_item(false)) {
			if (!v->get_next_unread(this)) {
				v->show_error(_("No unread items."));
			}
		}
		break;
	case OP_PREVUNREAD:
		LOG(level::INFO,
			"itemlist_formaction: jumping to previous unread item");
		if (!jump_to_previous_unread_item(false)) {
			if (!v->get_previous_unread(this)) {
				v->show_error(_("No unread items."));
			}
		}
		break;
	case OP_NEXT:
		LOG(level::INFO, "itemlist_formaction: jumping to next item");
		if (!jump_to_next_item(false)) {
			if (!v->get_next(this)) {
				v->show_error(_("Already on last item."));
			}
		}
		break;
	case OP_PREV:
		LOG(level::INFO,
			"itemlist_formaction: jumping to previous item");
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
		LOG(level::INFO, "itemlist_formaction: marking feed read");
		v->set_status(_("Marking feed read..."));
		try {
			if (feed->rssurl() != "") {
				v->get_ctrl()->mark_all_read(pos);
			} else {
				{
					std::lock_guard<std::mutex> lock(
						feed->item_mutex);
					LOG(level::DEBUG,
						"itemlist_formaction: oh, it "
						"looks "
						"like I'm in a pseudo-feed "
						"(search "
						"result, query feed)");
					for (const auto& item : feed->items()) {
						item->set_unread_nowrite_notify(
							false,
							true); // TODO: do we
							       // need to call
							       // mark_article_read
							       // here, too?
					}
				}
				v->get_ctrl()->mark_all_read(feed);
			}
			if (v->get_cfg()->get_configvalue_as_bool(
				    "markfeedread-jumps-to-next-unread"))
				process_operation(OP_NEXTUNREAD);
			invalidate(InvalidationMode::COMPLETE);
			v->set_status("");
		} catch (const dbexception& e) {
			v->show_error(strprintf::fmt(
				_("Error: couldn't mark feed read: %s"),
				e.what()));
		}
		break;
	case OP_MARKALLABOVEASREAD:
		LOG(level::INFO,
			"itemlist_formaction: marking all above as read");
		v->set_status(_("Marking all above as read..."));
		if (itemposname.length() > 0 &&
			itempos < visible_items.size()) {
			for (unsigned int i = 0; i < itempos; ++i) {
				if (visible_items[i].first->unread()) {
					visible_items[i].first->set_unread(
						false);
					v->get_ctrl()->mark_article_read(
						visible_items[i].first->guid(),
						true);
				}
			}
			if (!v->get_cfg()->get_configvalue_as_bool(
				    "show-read-articles")) {
				f->set("itempos", "0");
			}
			invalidate(InvalidationMode::COMPLETE);
		}
		v->set_status("");
		break;
	case OP_TOGGLESHOWREAD:
		m.parse(FILTER_UNREAD_ITEMS);
		LOG(level::DEBUG,
			"itemlist_formaction: toggling show-read-articles");
		if (v->get_cfg()->get_configvalue_as_bool(
			    "show-read-articles")) {
			v->get_cfg()->set_configvalue(
				"show-read-articles", "no");
			apply_filter = true;
		} else {
			v->get_cfg()->set_configvalue(
				"show-read-articles", "yes");
			apply_filter = false;
		}
		save_filterpos();
		invalidate(InvalidationMode::COMPLETE);
		break;
	case OP_PIPE_TO:
		if (visible_items.size() != 0) {
			std::vector<qna_pair> qna;
			if (automatic) {
				if (args->size() > 0) {
					qna_responses.clear();
					qna_responses.push_back((*args)[0]);
					finished_qna(OP_PIPE_TO);
				}
			} else {
				qna.push_back(qna_pair(
					_("Pipe article to command: "), ""));
				this->start_qna(
					qna, OP_PIPE_TO, &cmdlinehistory);
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
			this->start_qna(
				qna, OP_INT_START_SEARCH, &searchhistory);
		}
	} break;
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
				newfilter = v->select_filter(
					v->get_ctrl()
						->get_filters()
						.get_filters());
				LOG(level::DEBUG,
					"itemlist_formaction::run: newfilters "
					"= %s",
					newfilter);
			}
			if (newfilter != "") {
				filterhistory.add_line(newfilter);
				if (newfilter.length() > 0) {
					if (!m.parse(newfilter)) {
						v->show_error(strprintf::fmt(
							_("Error: couldn't "
							  "parse filter "
							  "command `%s': %s"),
							newfilter,
							m.get_parse_error()));
						m.parse(FILTER_UNREAD_ITEMS);
					} else {
						apply_filter = true;
						invalidate(InvalidationMode::
								COMPLETE);
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
			this->start_qna(
				qna, OP_INT_END_SETFILTER, &filterhistory);
		}
		break;
	case OP_CLEARFILTER:
		apply_filter = false;
		invalidate(InvalidationMode::COMPLETE);
		save_filterpos();
		break;
	case OP_SORT: {
		/// This string is related to the letters in parentheses in the
		/// "Sort by (d)ate/..." and "Reverse Sort by (d)ate/..."
		/// messages
		std::string input_options = _("dtfalg");
		char c = v->confirm(
			_("Sort by "
			  "(d)ate/(t)itle/(f)lags/(a)uthor/(l)ink/(g)uid?"),
			input_options);
		if (!c)
			break;
		unsigned int n_options = ((std::string) "dtfalg").length();
		if (input_options.length() < n_options)
			break;
		if (c == input_options.at(0)) {
			v->get_cfg()->set_configvalue(
				"article-sort-order", "date-asc");
		} else if (c == input_options.at(1)) {
			v->get_cfg()->set_configvalue(
				"article-sort-order", "title-asc");
		} else if (c == input_options.at(2)) {
			v->get_cfg()->set_configvalue(
				"article-sort-order", "flags-asc");
		} else if (c == input_options.at(3)) {
			v->get_cfg()->set_configvalue(
				"article-sort-order", "author-asc");
		} else if (c == input_options.at(4)) {
			v->get_cfg()->set_configvalue(
				"article-sort-order", "link-asc");
		} else if (c == input_options.at(5)) {
			v->get_cfg()->set_configvalue(
				"article-sort-order", "guid-asc");
		}
	} break;
	case OP_REVSORT: {
		std::string input_options = _("dtfalg");
		char c = v->confirm(
			_("Reverse Sort by "
			  "(d)ate/(t)itle/(f)lags/(a)uthor/(l)ink/(g)uid?"),
			input_options);
		if (!c)
			break;
		unsigned int n_options = ((std::string) "dtfalg").length();
		if (input_options.length() < n_options)
			break;
		if (c == input_options.at(0)) {
			v->get_cfg()->set_configvalue(
				"article-sort-order", "date-desc");
		} else if (c == input_options.at(1)) {
			v->get_cfg()->set_configvalue(
				"article-sort-order", "title-desc");
		} else if (c == input_options.at(2)) {
			v->get_cfg()->set_configvalue(
				"article-sort-order", "flags-desc");
		} else if (c == input_options.at(3)) {
			v->get_cfg()->set_configvalue(
				"article-sort-order", "author-desc");
		} else if (c == input_options.at(4)) {
			v->get_cfg()->set_configvalue(
				"article-sort-order", "link-desc");
		} else if (c == input_options.at(5)) {
			v->get_cfg()->set_configvalue(
				"article-sort-order", "guid-desc");
		}
	} break;
	case OP_INT_RESIZE:
		invalidate(InvalidationMode::COMPLETE);
		break;
	default:
		list_formaction::process_operation(op, automatic, args);
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

void itemlist_formaction::finished_qna(operation op)
{
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
			v->get_ctrl()->write_item(
				visible_items[itempos].first, ostr);
			v->push_empty_formaction();
			stfl::reset();
			FILE* f = popen(cmd.c_str(), "w");
			if (f) {
				std::string data = ostr.str();
				fwrite(data.c_str(), data.length(), 1, f);
				pclose(f);
			}
			v->pop_current_formaction();
		}
	} break;

	default:
		break;
	}
}

void itemlist_formaction::qna_end_setfilter()
{
	std::string filtertext = qna_responses[0];
	filterhistory.add_line(filtertext);

	if (filtertext.length() > 0) {
		if (!m.parse(filtertext)) {
			v->show_error(
				_("Error: couldn't parse filter command!"));
			return;
		}

		apply_filter = true;
		invalidate(InvalidationMode::COMPLETE);
		save_filterpos();
	}
}

void itemlist_formaction::qna_end_editflags()
{
	std::string itemposname = f->get("itempos");
	if (itemposname.length() == 0) {
		v->show_error(_("No item selected!")); // should not happen
		return;
	}

	unsigned int itempos = utils::to_u(itemposname);
	if (itempos < visible_items.size()) {
		visible_items[itempos].first->set_flags(qna_responses[0]);
		v->get_ctrl()->update_flags(visible_items[itempos].first);
		v->set_status(_("Flags updated."));
		LOG(level::DEBUG,
			"itemlist_formaction::finished_qna: updated flags");
		invalidate(itempos);
	}
}

void itemlist_formaction::qna_start_search()
{
	searchphrase = qna_responses[0];
	if (searchphrase.length() == 0)
		return;

	v->set_status(_("Searching..."));
	searchhistory.add_line(searchphrase);
	std::vector<std::shared_ptr<rss_item>> items;
	try {
		std::string utf8searchphrase = utils::convert_text(
			searchphrase, "utf-8", nl_langinfo(CODESET));
		if (show_searchresult) {
			items = v->get_ctrl()->search_for_items(
				utf8searchphrase, nullptr);
		} else {
			items = v->get_ctrl()->search_for_items(
				utf8searchphrase, feed);
		}
	} catch (const dbexception& e) {
		v->show_error(
			strprintf::fmt(_("Error while searching for `%s': %s"),
				searchphrase,
				e.what()));
		return;
	}

	if (items.empty()) {
		v->show_error(_("No results."));
		return;
	}

	{
		std::lock_guard<std::mutex> lock(search_dummy_feed->item_mutex);
		search_dummy_feed->clear_items();
		search_dummy_feed->add_items(items);
	}

	if (show_searchresult) {
		v->pop_current_formaction();
	}
	v->push_searchresult(search_dummy_feed, searchphrase);
}

void itemlist_formaction::do_update_visible_items()
{
	if (!(invalidated && invalidation_mode == InvalidationMode::COMPLETE))
		return;

	std::lock_guard<std::mutex> lock(feed->item_mutex);
	std::vector<std::shared_ptr<rss_item>>& items = feed->items();

	std::vector<itemptr_pos_pair> new_visible_items;

	/*
	 * this method doesn't redraw, all it does is to go through all
	 * items of a feed, and fill the visible_items vector by checking
	 * (if applicable) whether an items matches the currently active filter.
	 */

	unsigned int i = 0;
	for (const auto& item : items) {
		item->set_index(i + 1);
		if (!apply_filter || m.matches(item.get())) {
			new_visible_items.push_back(itemptr_pos_pair(item, i));
		}
		i++;
	}

	LOG(level::DEBUG,
		"itemlist_formaction::do_update_visible_items: size = %u",
		visible_items.size());

	visible_items = new_visible_items;
}

void itemlist_formaction::prepare()
{
	std::lock_guard<std::mutex> mtx(redraw_mtx);

	const auto sort_strategy = v->get_cfg()->get_article_sort_strategy();
	if (sort_strategy != old_sort_strategy) {
		feed->sort(sort_strategy);
		old_sort_strategy = sort_strategy;
		invalidate(InvalidationMode::COMPLETE);
	}

	try {
		do_update_visible_items();
	} catch (matcherexception& e) {
		v->show_error(strprintf::fmt(
			_("Error: applying the filter failed: %s"), e.what()));
		return;
	}

	if (v->get_cfg()->get_configvalue_as_bool("mark-as-read-on-hover")) {
		std::string itemposname = f->get("itempos");
		if (itemposname.length() > 0) {
			unsigned int itempos = utils::to_u(itemposname);
			if (visible_items[itempos].first->unread()) {
				visible_items[itempos].first->set_unread(false);
				v->get_ctrl()->mark_article_read(
					visible_items[itempos].first->guid(),
					true);
				invalidate(itempos);
			}
		}
	}

	unsigned int width = utils::to_u(f->get("items:w"));

	if (old_width != width) {
		invalidate(InvalidationMode::COMPLETE);
		old_width = width;
	}

	if (!invalidated)
		return;

	if (invalidated) {
		auto datetime_format =
			v->get_cfg()->get_configvalue("datetime-format");
		auto itemlist_format =
			v->get_cfg()->get_configvalue("articlelist-format");

		if (invalidation_mode == InvalidationMode::COMPLETE) {
			listfmt.clear();

			for (const auto& item : visible_items) {
				auto line = item2formatted_line(item,
					width,
					itemlist_format,
					datetime_format);
				listfmt.add_line(line, item.second);
			}
		} else if (invalidation_mode == InvalidationMode::PARTIAL) {
			for (const auto& itempos : invalidated_itempos) {
				auto item = visible_items[itempos];
				auto line = item2formatted_line(item,
					width,
					itemlist_format,
					datetime_format);
				listfmt.set_line(itempos, line, item.second);
			}
			invalidated_itempos.clear();
		} else {
			LOG(level::ERROR,
				"invalidation_mode is neither COMPLETE nor "
				"PARTIAL");
		}

		f->modify("items",
			"replace_inner",
			listfmt.format_list(rxman, "articlelist"));
	}

	invalidated = false;

	set_head(feed->title(),
		feed->unread_item_count(),
		feed->total_item_count(),
		feed->rssurl());

	prepare_set_filterpos();
}

std::string itemlist_formaction::item2formatted_line(
	const itemptr_pos_pair& item,
	const unsigned int width,
	const std::string& itemlist_format,
	const std::string& datetime_format)
{
	fmtstr_formatter fmt;
	fmt.register_fmt('i', strprintf::fmt("%u", item.second + 1));
	fmt.register_fmt('f', gen_flags(item.first));
	fmt.register_fmt('D',
		gen_datestr(item.first->pubDate_timestamp(), datetime_format));
	if (feed->rssurl() != item.first->feedurl() &&
		item.first->get_feedptr() != nullptr) {
		auto feedtitle = utils::replace_all(
			item.first->get_feedptr()->title(), "<", "<>");
		utils::remove_soft_hyphens(feedtitle);
		fmt.register_fmt('T', feedtitle);
	}

	auto itemtitle = utils::replace_all(item.first->title(), "<", "<>");
	utils::remove_soft_hyphens(itemtitle);
	fmt.register_fmt('t', itemtitle);

	auto itemauthor = utils::replace_all(item.first->author(), "<", "<>");
	utils::remove_soft_hyphens(itemauthor);
	fmt.register_fmt('a', itemauthor);

	fmt.register_fmt('L', item.first->length());

	auto formattedLine = fmt.do_format(itemlist_format, width);

	if (rxman) {
		int id;
		if ((id = rxman->article_matches(item.first.get())) != -1) {
			formattedLine =
				strprintf::fmt("<%d>%s</>", id, formattedLine);
		}
	}

	if (item.first->unread()) {
		formattedLine = strprintf::fmt("<unread>%s</>", formattedLine);
	}

	return formattedLine;
}

void itemlist_formaction::init()
{
	f->set("itempos", "0");
	f->set("msg", "");
	set_keymap_hints();
	apply_filter =
		!(v->get_cfg()->get_configvalue_as_bool("show-read-articles"));
	invalidate(InvalidationMode::COMPLETE);
	do_update_visible_items();
	if (v->get_cfg()->get_configvalue_as_bool("goto-first-unread")) {
		jump_to_next_unread_item(true);
	}
	f->run(-3); // FRUN - compute all widget dimensions
}

void itemlist_formaction::set_head(const std::string& s,
	unsigned int unread,
	unsigned int total,
	const std::string& url)
{
	/*
	 * Since the itemlist_formaction is also used to display search results,
	 * we always need to set the right title
	 */
	std::string title;
	fmtstr_formatter fmt;

	fmt.register_fmt('N', PROGRAM_NAME);
	fmt.register_fmt('V', PROGRAM_VERSION);

	fmt.register_fmt('u', std::to_string(unread));
	fmt.register_fmt('t', std::to_string(total));

	auto feedtitle = s;
	utils::remove_soft_hyphens(feedtitle);
	fmt.register_fmt('T', feedtitle);

	fmt.register_fmt('U', utils::censor_url(url));

	if (!show_searchresult) {
		title = fmt.do_format(v->get_cfg()->get_configvalue(
			"articlelist-title-format"));
	} else {
		title = fmt.do_format(v->get_cfg()->get_configvalue(
			"searchresult-title-format"));
	}
	f->set("head", title);
}

bool itemlist_formaction::jump_to_previous_unread_item(bool start_with_last)
{
	int itempos;
	std::istringstream is(f->get("itempos"));
	is >> itempos;
	for (int i = (start_with_last ? itempos : (itempos - 1)); i >= 0; --i) {
		LOG(level::DEBUG,
			"itemlist_formaction::jump_to_previous_unread_item: "
			"visible_items[%u] unread = %s",
			i,
			visible_items[i].first->unread() ? "true" : "false");
		if (visible_items[i].first->unread()) {
			f->set("itempos", std::to_string(i));
			return true;
		}
	}
	for (int i = visible_items.size() - 1; i >= itempos; --i) {
		if (visible_items[i].first->unread()) {
			f->set("itempos", std::to_string(i));
			return true;
		}
	}
	return false;
}

bool itemlist_formaction::jump_to_random_unread_item()
{
	bool has_unread_available = false;
	for (unsigned int i = 0; i < visible_items.size(); ++i) {
		if (visible_items[i].first->unread()) {
			has_unread_available = true;
			break;
		}
	}
	if (has_unread_available) {
		for (;;) {
			unsigned int pos =
				utils::get_random_value(visible_items.size());
			if (visible_items[pos].first->unread()) {
				f->set("itempos", std::to_string(pos));
				break;
			}
		}
	}
	return has_unread_available;
}

bool itemlist_formaction::jump_to_next_unread_item(bool start_with_first)
{
	unsigned int itempos = utils::to_u(f->get("itempos"));
	LOG(level::DEBUG,
		"itemlist_formaction::jump_to_next_unread_item: itempos = %u "
		"visible_items.size = %u",
		itempos,
		visible_items.size());
	for (unsigned int i = (start_with_first ? itempos : (itempos + 1));
		i < visible_items.size();
		++i) {
		LOG(level::DEBUG,
			"itemlist_formaction::jump_to_next_unread_item: i = %u",
			i);
		if (visible_items[i].first->unread()) {
			f->set("itempos", std::to_string(i));
			return true;
		}
	}
	for (unsigned int i = 0; i <= itempos && i < visible_items.size();
		++i) {
		LOG(level::DEBUG,
			"itemlist_formaction::jump_to_next_unread_item: i = %u",
			i);
		if (visible_items[i].first->unread()) {
			f->set("itempos", std::to_string(i));
			return true;
		}
	}
	return false;
}

bool itemlist_formaction::jump_to_previous_item(bool start_with_last)
{
	int itempos;
	std::istringstream is(f->get("itempos"));
	is >> itempos;

	int i = (start_with_last ? itempos : (itempos - 1));
	if (i >= 0) {
		LOG(level::DEBUG,
			"itemlist_formaction::jump_to_previous_item: "
			"visible_items[%u]",
			i);
		f->set("itempos", std::to_string(i));
		return true;
	}
	return false;
}

bool itemlist_formaction::jump_to_next_item(bool start_with_first)
{
	unsigned int itempos = utils::to_u(f->get("itempos"));
	LOG(level::DEBUG,
		"itemlist_formaction::jump_to_next_item: itempos = %u "
		"visible_items.size = %u",
		itempos,
		visible_items.size());
	unsigned int i = (start_with_first ? itempos : (itempos + 1));
	if (i < visible_items.size()) {
		LOG(level::DEBUG,
			"itemlist_formaction::jump_to_next_item: i = %u",
			i);
		f->set("itempos", std::to_string(i));
		return true;
	}
	return false;
}

std::string itemlist_formaction::get_guid()
{
	unsigned int itempos = utils::to_u(f->get("itempos"));
	return visible_items[itempos].first->guid();
}

keymap_hint_entry* itemlist_formaction::get_keymap_hint()
{
	static keymap_hint_entry hints[] = {{OP_QUIT, _("Quit")},
		{OP_OPEN, _("Open")},
		{OP_SAVE, _("Save")},
		{OP_RELOAD, _("Reload")},
		{OP_NEXTUNREAD, _("Next Unread")},
		{OP_MARKFEEDREAD, _("Mark All Read")},
		{OP_SEARCH, _("Search")},
		{OP_HELP, _("Help")},
		{OP_NIL, nullptr}};
	return hints;
}

void itemlist_formaction::handle_cmdline_num(unsigned int idx)
{
	if (idx > 0 &&
		idx <= visible_items[visible_items.size() - 1].second + 1) {
		int i = get_pos(idx - 1);
		if (i == -1) {
			v->show_error(_("Position not visible!"));
		} else {
			f->set("itempos", std::to_string(i));
		}
	} else {
		v->show_error(_("Invalid position!"));
	}
}

void itemlist_formaction::handle_cmdline(const std::string& cmd)
{
	unsigned int idx = 0;
	if (1 == sscanf(cmd.c_str(), "%u", &idx)) {
		handle_cmdline_num(idx);
	} else {
		std::vector<std::string> tokens = utils::tokenize_quoted(cmd);
		if (tokens.empty())
			return;
		if (tokens[0] == "save" && tokens.size() >= 2) {
			std::string filename = utils::resolve_tilde(tokens[1]);
			std::string itemposname = f->get("itempos");
			LOG(level::INFO,
				"itemlist_formaction::handle_cmdline: saving "
				"item "
				"at pos `%s' to `%s'",
				itemposname,
				filename);
			if (itemposname.length() > 0) {
				unsigned int itempos = utils::to_u(itemposname);
				save_article(
					filename, visible_items[itempos].first);
			} else {
				v->show_error(_("Error: no item selected!"));
			}
		} else {
			formaction::handle_cmdline(cmd);
		}
	}
}

int itemlist_formaction::get_pos(unsigned int realidx)
{
	for (unsigned int i = 0; i < visible_items.size(); ++i) {
		if (visible_items[i].second == realidx)
			return i;
	}
	return -1;
}

void itemlist_formaction::recalculate_form()
{
	formaction::recalculate_form();
	invalidate(InvalidationMode::COMPLETE);

	std::string itemposname = f->get("itempos");
	unsigned int itempos = utils::to_u(itemposname);

	// If the old position was set and it is less than the itempos, use it
	// for the feed's itempos Correct the problem when you open itemview and
	// jump to next then exit to itemlist and the itempos is wrong This only
	// applies when "show-read-articles" is set to false
	if ((old_itempos != -1) && itempos > (unsigned int)old_itempos &&
		!v->get_cfg()->get_configvalue_as_bool("show-read-articles")) {
		f->set("itempos", strprintf::fmt("%u", old_itempos));
		old_itempos = -1; // Reset
	}
}

void itemlist_formaction::save_article(const std::string& filename,
	std::shared_ptr<rss_item> item)
{
	if (filename == "") {
		v->show_error(_("Aborted saving."));
	} else {
		try {
			v->get_ctrl()->write_item(item, filename);
			v->show_error(strprintf::fmt(
				_("Saved article to %s"), filename));
		} catch (...) {
			v->show_error(strprintf::fmt(
				_("Error: couldn't save article to %s"),
				filename));
		}
	}
}

void itemlist_formaction::save_filterpos()
{
	unsigned int i = utils::to_u(f->get("itempos"));
	if (i < visible_items.size()) {
		filterpos = visible_items[i].second;
		set_filterpos = true;
	}
}

void itemlist_formaction::set_regexmanager(regexmanager* r)
{
	rxman = r;
	std::vector<std::string>& attrs = r->get_attrs("articlelist");
	unsigned int i = 0;
	std::string attrstr;
	for (const auto& attribute : attrs) {
		attrstr.append(
			strprintf::fmt("@style_%u_normal:%s ", i, attribute));
		attrstr.append(
			strprintf::fmt("@style_%u_focus:%s ", i, attribute));
		i++;
	}
	std::string textview = strprintf::fmt(
		"{list[items] .expand:vh style_normal[listnormal]: "
		"style_focus[listfocus]:fg=yellow,bg=blue,attr=bold "
		"pos_name[itemposname]: pos[itempos]:0 %s richtext:1}",
		attrstr);
	f->modify("items", "replace", textview);
}

std::string itemlist_formaction::gen_flags(std::shared_ptr<rss_item> item)
{
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

std::string itemlist_formaction::gen_datestr(time_t t,
	const std::string& datetimeformat)
{
	char datebuf[64];
	struct tm* stm = localtime(&t);
	strftime(datebuf, sizeof(datebuf), datetimeformat.c_str(), stm);
	return datebuf;
}

void itemlist_formaction::prepare_set_filterpos()
{
	if (set_filterpos) {
		set_filterpos = false;
		unsigned int i = 0;
		for (const auto& item : visible_items) {
			if (item.second == filterpos) {
				f->set("itempos", std::to_string(i));
				return;
			}
			i++;
		}
		f->set("itempos", "0");
	}
}

void itemlist_formaction::set_feed(std::shared_ptr<rss_feed> fd)
{
	LOG(level::DEBUG,
		"itemlist_formaction::set_feed: fd pointer = %p title = `%s'",
		fd.get(),
		fd->title());
	feed = fd;
	feed->load();
	invalidate(InvalidationMode::COMPLETE);
	do_update_visible_items();
}

std::string itemlist_formaction::title()
{
	if (feed->rssurl() == "") {
		return strprintf::fmt(_("Search Result - '%s'"), searchphrase);
	} else {
		if (feed->is_query_feed()) {
			return strprintf::fmt(_("Query Feed - %s"),
				feed->rssurl().substr(
					6, feed->rssurl().length() - 6));
		} else {
			auto feedtitle = feed->title();
			utils::remove_soft_hyphens(feedtitle);
			return strprintf::fmt(
				_("Article List - %s"), feedtitle);
		}
	}
}

} // namespace newsboat

#include <itemlistformaction.h>

#include <cassert>
#include <cinttypes>
#include <cstdio>
#include <langinfo.h>
#include <sstream>
#include <sys/stat.h>

#include "config.h"
#include "controller.h"
#include "dbexception.h"
#include "fmtstrformatter.h"
#include "logger.h"
#include "matcherexception.h"
#include "rssfeed.h"
#include "scopemeasure.h"
#include "strprintf.h"
#include "utils.h"
#include "view.h"

namespace newsboat {

ItemListFormAction::ItemListFormAction(View* vv,
	std::string formstr,
	Cache* cc,
	FilterContainer* f,
	ConfigContainer* cfg)
	: ListFormAction(vv, formstr, cfg)
	, pos(0)
	, apply_filter(false)
	, show_searchresult(false)
	, search_dummy_feed(new RssFeed(cc))
	, set_filterpos(false)
	, filterpos(0)
	, rxman(0)
	, old_width(0)
	, old_itempos(-1)
	, old_sort_strategy({ArtSortMethod::TITLE, SortDirection::DESC})
	, invalidated(false)
	, invalidation_mode(InvalidationMode::COMPLETE)
	, rsscache(cc)
	, filters(f)
{
	search_dummy_feed->set_search_feed(true);
}

ItemListFormAction::~ItemListFormAction() {}

void ItemListFormAction::process_operation(Operation op,
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
		LOG(Level::INFO,
			"ItemListFormAction: opening item at pos `%s'",
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
		ScopeMeasure m1("OP_DELETE");
		if (itemposname.length() > 0 && visible_items.size() != 0) {
			// mark as read
			v->get_ctrl()->mark_article_read(
				visible_items[itempos].first->guid(), true);
			visible_items[itempos].first->set_unread(false);
			// mark as deleted
			visible_items[itempos].first->set_deleted(
				!visible_items[itempos].first->deleted());
			rsscache->mark_item_deleted(
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
		ScopeMeasure m1("OP_DELETE_ALL");
		if (visible_items.size() > 0) {
			v->get_ctrl()->mark_all_read(pos);
			for (const auto& pair : visible_items) {
				pair.first->set_deleted(true);
			}
			if (feed->is_query_feed()) {
				for (const auto& pair : visible_items) {
					rsscache->mark_item_deleted(
						pair.first->guid(), true);
				}
			} else {
				rsscache->mark_feed_items_deleted(
					feed->rssurl());
			}
			invalidate_everything();
		}
	} break;
	case OP_PURGE_DELETED: {
		ScopeMeasure m1("OP_PURGE_DELETED");
		feed->purge_deleted_items();
		invalidate_everything();
	} break;
	case OP_OPENBROWSER_AND_MARK: {
		LOG(Level::INFO,
			"ItemListFormAction: opening item at pos `%s'",
			itemposname);
		if (itemposname.length() > 0 && visible_items.size() != 0) {
			if (itempos < visible_items.size()) {
				visible_items[itempos].first->set_unread(false);
				v->get_ctrl()->mark_article_read(
					visible_items[itempos].first->guid(),
					true);
				v->open_in_browser(
					visible_items[itempos].first->link());
				if (!cfg->get_configvalue_as_bool(
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
		LOG(Level::INFO,
			"ItemListFormAction: opening item at pos `%s'",
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
			LOG(Level::INFO,
				"ItemListFormAction: opening all unread items "
				"in "
				"browser");
			open_unread_items_in_browser(feed, false);
		}
	} break;
	case OP_OPENALLUNREADINBROWSER_AND_MARK: {
		if (feed) {
			LOG(Level::INFO,
				"ItemListFormAction: opening all unread items "
				"in "
				"browser and marking read");
			open_unread_items_in_browser(feed, true);
			invalidate_everything();
		}
	} break;
	case OP_TOGGLEITEMREAD: {
		LOG(Level::INFO,
			"ItemListFormAction: toggling item read at pos `%s'",
			itemposname);
		if (itemposname.length() > 0 && visible_items.size() != 0) {
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
					rsscache->mark_item_deleted(
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
			} catch (const DbException& e) {
				v->set_status(strprintf::fmt(
					_("Error while toggling read flag: %s"),
					e.what()));
			}
			if (!cfg->get_configvalue_as_bool(
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
				std::string urlviewer = cfg->get_configvalue(
					"external-url-viewer");
				if (urlviewer == "") {
					std::vector<LinkPair> links;
					std::vector<std::pair<LineType,
						std::string>>
						lines;
					HtmlRenderer rnd;
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
		LOG(Level::INFO,
			"ItemListFormAction: bookmarking item at pos `%s'",
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
					std::vector<QnaPair> qna;
					qna.push_back(QnaPair(_("Flags: "),
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
		LOG(Level::INFO,
			"ItemListFormAction: saving item at pos `%s'",
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
	case OP_SAVEALL:
		handle_op_saveall();
		break;
	case OP_HELP:
		v->push_help();
		break;
	case OP_RELOAD:
		if (!show_searchresult) {
			LOG(Level::INFO,
				"ItemListFormAction: reloading current feed");
			v->get_ctrl()->get_reloader()->reload(pos);
			invalidate_everything();
		} else {
			v->show_error(
				_("Error: you can't reload search results."));
		}
		break;
	case OP_QUIT:
		LOG(Level::INFO, "ItemListFormAction: quitting");
		v->feedlist_mark_pos_if_visible(pos);
		feed->purge_deleted_items();
		feed->unload();
		quit = true;
		break;
	case OP_HARDQUIT:
		LOG(Level::INFO, "ItemListFormAction: hard quitting");
		v->feedlist_mark_pos_if_visible(pos);
		feed->purge_deleted_items();
		hardquit = true;
		break;
	case OP_NEXTUNREAD:
		LOG(Level::INFO,
			"ItemListFormAction: jumping to next unread item");
		if (!jump_to_next_unread_item(false)) {
			if (!v->get_next_unread(this)) {
				v->show_error(_("No unread items."));
			}
		}
		break;
	case OP_PREVUNREAD:
		LOG(Level::INFO,
			"ItemListFormAction: jumping to previous unread item");
		if (!jump_to_previous_unread_item(false)) {
			if (!v->get_previous_unread(this)) {
				v->show_error(_("No unread items."));
			}
		}
		break;
	case OP_NEXT:
		LOG(Level::INFO, "ItemListFormAction: jumping to next item");
		if (!jump_to_next_item(false)) {
			if (!v->get_next(this)) {
				v->show_error(_("Already on last item."));
			}
		}
		break;
	case OP_PREV:
		LOG(Level::INFO,
			"ItemListFormAction: jumping to previous item");
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
		LOG(Level::INFO, "ItemListFormAction: marking feed read");
		v->set_status(_("Marking feed read..."));
		try {
			if (feed->rssurl() != "") {
				v->get_ctrl()->mark_all_read(pos);
			} else {
				{
					std::lock_guard<std::mutex> lock(
						feed->item_mutex);
					LOG(Level::DEBUG,
						"ItemListFormAction: oh, it "
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
			if (cfg->get_configvalue_as_bool(
				    "markfeedread-jumps-to-next-unread")) {
				process_operation(OP_NEXTUNREAD);
			} else { // reposition to first/last item
				std::string sortorder = cfg->get_configvalue(
					"article-sort-order");

				if (sortorder == "date-desc") {
					LOG(Level::DEBUG,
						"ItemListFormAction:: "
						"reset itempos to last");
					f->set("itempos",
						std::to_string(
							visible_items.size() -
							1));
				}
				if (sortorder == "date-asc") {
					LOG(Level::DEBUG,
						"ItemListFormAction:: "
						"reset itempos to first");
					f->set("itempos", "0");
				}
			}
			invalidate_everything();
			v->set_status("");
		} catch (const DbException& e) {
			v->show_error(strprintf::fmt(
				_("Error: couldn't mark feed read: %s"),
				e.what()));
		}
		break;
	case OP_MARKALLABOVEASREAD:
		LOG(Level::INFO,
			"ItemListFormAction: marking all above as read");
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
			if (!cfg->get_configvalue_as_bool(
				    "show-read-articles")) {
				f->set("itempos", "0");
			}
			invalidate_everything();
		}
		v->set_status("");
		break;
	case OP_TOGGLESHOWREAD:
		LOG(Level::DEBUG,
			"ItemListFormAction: toggling show-read-articles");
		if (cfg->get_configvalue_as_bool("show-read-articles")) {
			cfg->set_configvalue("show-read-articles", "no");
		} else {
			cfg->set_configvalue("show-read-articles", "yes");
		}
		save_filterpos();
		invalidate_everything();
		break;
	case OP_PIPE_TO:
		if (visible_items.size() != 0) {
			std::vector<QnaPair> qna;
			if (automatic) {
				if (args->size() > 0) {
					qna_responses.clear();
					qna_responses.push_back((*args)[0]);
					finished_qna(OP_PIPE_TO);
				}
			} else {
				qna.push_back(QnaPair(
					_("Pipe article to command: "), ""));
				this->start_qna(
					qna, OP_PIPE_TO, &cmdlinehistory);
			}
		} else {
			v->show_error(_("No item selected!"));
		}
		break;
	case OP_SEARCH: {
		std::vector<QnaPair> qna;
		if (automatic) {
			if (args->size() > 0) {
				qna_responses.clear();
				qna_responses.push_back((*args)[0]);
				finished_qna(OP_INT_START_SEARCH);
			}
		} else {
			qna.push_back(QnaPair(_("Search for: "), ""));
			this->start_qna(
				qna, OP_INT_START_SEARCH, &searchhistory);
		}
	} break;
	case OP_EDIT_URLS:
		v->get_ctrl()->edit_urls_file();
		break;
	case OP_SELECTFILTER:
		if (filters->size() > 0) {
			std::string newfilter;
			if (automatic) {
				if (args->size() > 0)
					newfilter = (*args)[0];
			} else {
				newfilter = v->select_filter(
					filters->get_filters());
				LOG(Level::DEBUG,
					"ItemListFormAction::run: newfilters "
					"= %s",
					newfilter);
			}
			if (newfilter != "") {
				filterhistory.add_line(newfilter);
				if (newfilter.length() > 0) {
					if (!matcher.parse(newfilter)) {
						v->show_error(strprintf::fmt(
							_("Error: couldn't "
							  "parse filter "
							  "command `%s': %s"),
							newfilter,
							matcher.get_parse_error()));
					} else {
						apply_filter = true;
						invalidate_everything();
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
			std::vector<QnaPair> qna;
			qna.push_back(QnaPair(_("Filter: "), ""));
			this->start_qna(
				qna, OP_INT_END_SETFILTER, &filterhistory);
		}
		break;
	case OP_CLEARFILTER:
		apply_filter = false;
		invalidate_everything();
		save_filterpos();
		break;
	case OP_SORT: {
		/// This string is related to the letters in parentheses in the
		/// "Sort by (d)ate/..." and "Reverse Sort by (d)ate/..."
		/// messages
		std::string input_options = _("dtfalgr");
		char c = v->confirm(_("Sort by "
				      "(d)ate/(t)itle/(f)lags/(a)uthor/(l)ink/"
				      "(g)uid/(r)andom?"),
			input_options);
		if (!c) {
			break;
		}

		// Check that the number of translated answers is the same as
		// the number of answers we expect to handle. If it doesn't,
		// just give up. That'll prevent this function from sorting
		// anything, so users will complain, and we'll ask them to
		// update the translation. A bit lame, but it's better than
		// mishandling the answer.
		const auto n_options = ((std::string) "dtfalgr").length();
		if (input_options.length() < n_options) {
			break;
		}

		if (c == input_options.at(0)) {
			cfg->set_configvalue("article-sort-order", "date-asc");
		} else if (c == input_options.at(1)) {
			cfg->set_configvalue("article-sort-order", "title-asc");
		} else if (c == input_options.at(2)) {
			cfg->set_configvalue("article-sort-order", "flags-asc");
		} else if (c == input_options.at(3)) {
			cfg->set_configvalue(
				"article-sort-order", "author-asc");
		} else if (c == input_options.at(4)) {
			cfg->set_configvalue("article-sort-order", "link-asc");
		} else if (c == input_options.at(5)) {
			cfg->set_configvalue("article-sort-order", "guid-asc");
		} else if (c == input_options.at(6)) {
			cfg->set_configvalue("article-sort-order", "random");
		}
	} break;
	case OP_REVSORT: {
		std::string input_options = _("dtfalgr");
		char c = v->confirm(_("Reverse Sort by "
				      "(d)ate/(t)itle/(f)lags/(a)uthor/(l)ink/"
				      "(g)uid/(r)andom?"),
			input_options);
		if (!c) {
			break;
		}

		// Check that the number of translated answers is the same as
		// the number of answers we expect to handle. If it doesn't,
		// just give up. That'll prevent this function from sorting
		// anything, so users will complain, and we'll ask them to
		// update the translation. A bit lame, but it's better than
		// mishandling the answer.
		const auto n_options = ((std::string) "dtfalgr").length();
		if (input_options.length() < n_options) {
			break;
		}

		if (c == input_options.at(0)) {
			cfg->set_configvalue("article-sort-order", "date-desc");
		} else if (c == input_options.at(1)) {
			cfg->set_configvalue(
				"article-sort-order", "title-desc");
		} else if (c == input_options.at(2)) {
			cfg->set_configvalue(
				"article-sort-order", "flags-desc");
		} else if (c == input_options.at(3)) {
			cfg->set_configvalue(
				"article-sort-order", "author-desc");
		} else if (c == input_options.at(4)) {
			cfg->set_configvalue("article-sort-order", "link-desc");
		} else if (c == input_options.at(5)) {
			cfg->set_configvalue("article-sort-order", "guid-desc");
		} else if (c == input_options.at(6)) {
			cfg->set_configvalue("article-sort-order", "random");
		}
	} break;
	case OP_INT_RESIZE:
		invalidate_everything();
		break;
	default:
		ListFormAction::process_operation(op, automatic, args);
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

void ItemListFormAction::finished_qna(Operation op)
{
	FormAction::finished_qna(op); // important!

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
			Stfl::reset();
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

void ItemListFormAction::qna_end_setfilter()
{
	std::string filtertext = qna_responses[0];
	filterhistory.add_line(filtertext);

	if (filtertext.length() > 0) {
		if (!matcher.parse(filtertext)) {
			v->show_error(
				_("Error: couldn't parse filter command!"));
			return;
		}

		apply_filter = true;
		invalidate_everything();
		save_filterpos();
	}
}

void ItemListFormAction::qna_end_editflags()
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
		LOG(Level::DEBUG,
			"ItemListFormAction::finished_qna: updated flags");
		invalidate(itempos);
	}
}

void ItemListFormAction::qna_start_search()
{
	searchphrase = qna_responses[0];
	if (searchphrase.length() == 0)
		return;

	v->set_status(_("Searching..."));
	searchhistory.add_line(searchphrase);
	std::vector<std::shared_ptr<RssItem>> items;
	try {
		std::string utf8searchphrase = utils::convert_text(
			searchphrase, "utf-8", nl_langinfo(CODESET));
		if (show_searchresult)
			feed->set_rssurl("search:");
		items = v->get_ctrl()->search_for_items(utf8searchphrase, feed);
	} catch (const DbException& e) {
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

void ItemListFormAction::do_update_visible_items()
{
	if (!(invalidated && invalidation_mode == InvalidationMode::COMPLETE))
		return;

	std::lock_guard<std::mutex> lock(feed->item_mutex);
	std::vector<std::shared_ptr<RssItem>>& items = feed->items();

	std::vector<ItemPtrPosPair> new_visible_items;

	/*
	 * this method doesn't redraw, all it does is to go through all
	 * items of a feed, and fill the visible_items vector by checking
	 * (if applicable) whether an items matches the currently active filter.
	 */

	bool show_read = cfg->get_configvalue_as_bool("show-read-articles");

	unsigned int i = 0;
	for (const auto& item : items) {
		item->set_index(i + 1);
		if ((show_read || item->unread()) &&
			(!apply_filter || matcher.matches(item.get()))) {
			new_visible_items.push_back(ItemPtrPosPair(item, i));
		}
		i++;
	}

	LOG(Level::DEBUG,
		"ItemListFormAction::do_update_visible_items: size = %" PRIu64,
		static_cast<uint64_t>(visible_items.size()));

	visible_items = new_visible_items;
}

void ItemListFormAction::prepare()
{
	std::lock_guard<std::mutex> mtx(redraw_mtx);

	const auto sort_strategy = cfg->get_article_sort_strategy();
	if (sort_strategy != old_sort_strategy) {
		feed->sort(sort_strategy);
		old_sort_strategy = sort_strategy;
		invalidate_everything();
	}

	try {
		do_update_visible_items();
	} catch (MatcherException& e) {
		v->show_error(strprintf::fmt(
			_("Error: applying the filter failed: %s"), e.what()));
		return;
	}

	if (cfg->get_configvalue_as_bool("mark-as-read-on-hover")) {
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
		invalidate_everything();
		old_width = width;
	}

	if (!invalidated) {
		return;
	}

	auto datetime_format = cfg->get_configvalue("datetime-format");
	auto itemlist_format = cfg->get_configvalue("articlelist-format");

	switch (invalidation_mode) {
	case InvalidationMode::COMPLETE:
		listfmt.clear();

		for (const auto& item : visible_items) {
			auto line = item2formatted_line(
				item, width, itemlist_format, datetime_format);
			listfmt.add_line(line, item.second);
		}
		break;

	case InvalidationMode::PARTIAL:
		for (const auto& itempos : invalidated_itempos) {
			auto item = visible_items[itempos];
			auto line = item2formatted_line(
				item, width, itemlist_format, datetime_format);
			listfmt.set_line(itempos, line, item.second);
		}
		break;
	}

	f->modify("items",
		"replace_inner",
		listfmt.format_list(rxman, "articlelist"));

	invalidated_itempos.clear();
	invalidated = false;

	set_head(feed->title(),
		feed->unread_item_count(),
		feed->total_item_count(),
		feed->rssurl());

	prepare_set_filterpos();
}

std::string ItemListFormAction::item2formatted_line(const ItemPtrPosPair& item,
	const unsigned int width,
	const std::string& itemlist_format,
	const std::string& datetime_format)
{
	FmtStrFormatter fmt;
	fmt.register_fmt('i', strprintf::fmt("%u", item.second + 1));
	fmt.register_fmt('f', gen_flags(item.first));
	fmt.register_fmt('D',
		gen_datestr(item.first->pubDate_timestamp(), datetime_format));
	if (feed->rssurl() != item.first->feedurl() &&
		item.first->get_feedptr() != nullptr) {
		auto feedtitle = utils::quote_for_stfl(
			item.first->get_feedptr()->title());
		utils::remove_soft_hyphens(feedtitle);
		fmt.register_fmt('T', feedtitle);
	}

	auto itemtitle = utils::quote_for_stfl(item.first->title());
	utils::remove_soft_hyphens(itemtitle);
	fmt.register_fmt('t', itemtitle);

	auto itemauthor = utils::quote_for_stfl(item.first->author());
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

void ItemListFormAction::init()
{
	f->set("itempos", "0");
	f->set("msg", "");
	set_keymap_hints();
	invalidate_everything();
	do_update_visible_items();
	if (cfg->get_configvalue_as_bool("goto-first-unread")) {
		jump_to_next_unread_item(true);
	}
	f->run(-3); // FRUN - compute all widget dimensions
}

void ItemListFormAction::set_head(const std::string& s,
	unsigned int unread,
	unsigned int total,
	const std::string& url)
{
	/*
	 * Since the ItemListFormAction is also used to display search results,
	 * we always need to set the right title
	 */
	std::string title;
	FmtStrFormatter fmt;

	fmt.register_fmt('N', PROGRAM_NAME);
	fmt.register_fmt('V', utils::program_version());

	fmt.register_fmt('u', std::to_string(unread));
	fmt.register_fmt('t', std::to_string(total));

	auto feedtitle = s;
	utils::remove_soft_hyphens(feedtitle);
	fmt.register_fmt('T', feedtitle);

	fmt.register_fmt('U', utils::censor_url(url));

	if (!show_searchresult) {
		title = fmt.do_format(
			cfg->get_configvalue("articlelist-title-format"));
	} else {
		title = fmt.do_format(
			cfg->get_configvalue("searchresult-title-format"));
	}
	f->set("head", title);
}

bool ItemListFormAction::jump_to_previous_unread_item(bool start_with_last)
{
	int itempos;
	std::istringstream is(f->get("itempos"));
	is >> itempos;
	for (int i = (start_with_last ? itempos : (itempos - 1)); i >= 0; --i) {
		LOG(Level::DEBUG,
			"ItemListFormAction::jump_to_previous_unread_item: "
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

bool ItemListFormAction::jump_to_random_unread_item()
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

bool ItemListFormAction::jump_to_next_unread_item(bool start_with_first)
{
	unsigned int itempos = utils::to_u(f->get("itempos"));
	LOG(Level::DEBUG,
		"ItemListFormAction::jump_to_next_unread_item: itempos = %u "
		"visible_items.size = %" PRIu64,
		itempos,
		static_cast<uint64_t>(visible_items.size()));
	for (unsigned int i = (start_with_first ? itempos : (itempos + 1));
		i < visible_items.size();
		++i) {
		LOG(Level::DEBUG,
			"ItemListFormAction::jump_to_next_unread_item: i = %u",
			i);
		if (visible_items[i].first->unread()) {
			f->set("itempos", std::to_string(i));
			return true;
		}
	}
	for (unsigned int i = 0; i <= itempos && i < visible_items.size();
		++i) {
		LOG(Level::DEBUG,
			"ItemListFormAction::jump_to_next_unread_item: i = %u",
			i);
		if (visible_items[i].first->unread()) {
			f->set("itempos", std::to_string(i));
			return true;
		}
	}
	return false;
}

bool ItemListFormAction::jump_to_previous_item(bool start_with_last)
{
	int itempos;
	std::istringstream is(f->get("itempos"));
	is >> itempos;

	int i = (start_with_last ? itempos : (itempos - 1));
	if (i >= 0) {
		LOG(Level::DEBUG,
			"ItemListFormAction::jump_to_previous_item: "
			"visible_items[%u]",
			i);
		f->set("itempos", std::to_string(i));
		return true;
	}
	return false;
}

bool ItemListFormAction::jump_to_next_item(bool start_with_first)
{
	unsigned int itempos = utils::to_u(f->get("itempos"));
	LOG(Level::DEBUG,
		"ItemListFormAction::jump_to_next_item: itempos = %" PRIu64
		" visible_items.size = %" PRIu64,
		static_cast<uint64_t>(itempos),
		static_cast<uint64_t>(visible_items.size()));
	unsigned int i = (start_with_first ? itempos : (itempos + 1));
	if (i < visible_items.size()) {
		LOG(Level::DEBUG,
			"ItemListFormAction::jump_to_next_item: i = %u",
			i);
		f->set("itempos", std::to_string(i));
		return true;
	}
	return false;
}

std::string ItemListFormAction::get_guid()
{
	unsigned int itempos = utils::to_u(f->get("itempos"));
	return visible_items[itempos].first->guid();
}

KeyMapHintEntry* ItemListFormAction::get_keymap_hint()
{
	static KeyMapHintEntry hints[] = {{OP_QUIT, _("Quit")},
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

void ItemListFormAction::handle_cmdline_num(unsigned int idx)
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

void ItemListFormAction::handle_cmdline(const std::string& cmd)
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
			LOG(Level::INFO,
				"ItemListFormAction::handle_cmdline: saving "
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
			FormAction::handle_cmdline(cmd);
		}
	}
}

int ItemListFormAction::get_pos(unsigned int realidx)
{
	for (unsigned int i = 0; i < visible_items.size(); ++i) {
		if (visible_items[i].second == realidx)
			return i;
	}
	return -1;
}

void ItemListFormAction::recalculate_form()
{
	FormAction::recalculate_form();
	invalidate_everything();

	std::string itemposname = f->get("itempos");
	unsigned int itempos = utils::to_u(itemposname);

	// If the old position was set and it is less than the itempos, use it
	// for the feed's itempos Correct the problem when you open itemview and
	// jump to next then exit to itemlist and the itempos is wrong This only
	// applies when "show-read-articles" is set to false
	if ((old_itempos != -1) && itempos > (unsigned int)old_itempos &&
		!cfg->get_configvalue_as_bool("show-read-articles")) {
		f->set("itempos", strprintf::fmt("%u", old_itempos));
		old_itempos = -1; // Reset
	}
}

void ItemListFormAction::save_article(const std::string& filename,
	std::shared_ptr<RssItem> item)
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

void ItemListFormAction::save_filterpos()
{
	unsigned int i = utils::to_u(f->get("itempos"));
	if (i < visible_items.size()) {
		filterpos = visible_items[i].second;
		set_filterpos = true;
	}
}

void ItemListFormAction::set_regexmanager(RegexManager* r)
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

std::string ItemListFormAction::gen_flags(std::shared_ptr<RssItem> item)
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

std::string ItemListFormAction::gen_datestr(time_t t,
	const std::string& datetimeformat)
{
	char datebuf[64];
	struct tm* stm = localtime(&t);
	strftime(datebuf, sizeof(datebuf), datetimeformat.c_str(), stm);
	return datebuf;
}

void ItemListFormAction::prepare_set_filterpos()
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

void ItemListFormAction::set_feed(std::shared_ptr<RssFeed> fd)
{
	LOG(Level::DEBUG,
		"ItemListFormAction::set_feed: fd pointer = %p title = `%s'",
		fd.get(),
		fd->title());
	feed = fd;
	feed->load();
	invalidate_everything();
	do_update_visible_items();
}

std::string ItemListFormAction::title()
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

void ItemListFormAction::handle_op_saveall()
{
	LOG(Level::INFO, "ItemListFormAction: saving all items");

	if (visible_items.empty()) {
		return;
	}

	std::string directory = v->run_dirbrowser();

	if (directory.empty()) {
		return;
	}

	if (directory.back() != '/') {
		directory = directory + "/";
	}

	if (visible_items.size() == 1) {
		const std::string filename = v->get_filename_suggestion(
			visible_items[0].first->title());
		const std::string fpath = directory + filename;

		struct stat sbuf;
		if (::stat(fpath.c_str(), &sbuf) != -1) {
			std::string input_options = _("yn");
			char c = v->confirm(
				strprintf::fmt(_("Overwrite `%s' in `%s?' "
						 "(y:Yes n:No)"),
					filename,
					directory),
				input_options);
			if (!c) {
				return;
			}

			// Check that the number of translated answers is the
			// same as the number of answers we expect to handle. If
			// it doesn't, just give up. That'll prevent this
			// function from saving anything, so users will
			// complain, and we'll ask them to update the
			// translation. A bit lame, but it's better than
			// mishandling the answer.
			const auto n_options = ((std::string) "yn").length();
			if (input_options.length() < n_options) {
				return;
			}

			if (c == input_options.at(0)) {
				save_article(fpath, visible_items[0].first);
			} else if (c == input_options.at(1)) {
				return;
			}
		} else {
			// Create file since it does not exist
			save_article(fpath, visible_items[0].first);
		}
	} else {
		std::vector<std::string> filenames;
		for (const auto& item : visible_items) {
			filenames.emplace_back(v->get_filename_suggestion(
				item.first->title()));
		}

		const auto unique_filenames = std::set<std::string>(
			std::begin(filenames), std::end(filenames));

		int nfiles_exist = filenames.size() - unique_filenames.size();
		for (const auto& filename : unique_filenames) {
			const auto filepath = directory + filename;
			struct stat sbuf;
			if (::stat(filepath.c_str(), &sbuf) != -1) {
				nfiles_exist++;
			}
		}

		// Check that the number of translated answers is the same as
		// the number of answers we expect to handle. If it doesn't,
		// just give up. That'll prevent this function from saving
		// anything, so users will complain, and we'll ask them to
		// update the translation. A bit lame, but it's better than
		// mishandling the answer.
		const std::string input_options = _("yanq");
		const auto n_options = ((std::string) "yanq").length();
		if (input_options.length() < n_options) {
			return;
		}

		bool overwrite_all = false;
		for (size_t item_idx = 0; item_idx < filenames.size();
			++item_idx) {
			const auto filename = filenames[item_idx];
			const auto filepath = directory + filename;
			auto item = visible_items[item_idx].first;

			struct stat sbuf;
			if (::stat(filepath.c_str(), &sbuf) != -1) {
				if (overwrite_all) {
					save_article(filepath, item);
					continue;
				}

				char c;
				if (nfiles_exist > 1) {
					c = v->confirm(
						strprintf::fmt(
							_("Overwrite `%s' in "
							  "`%s'? "
							  "There are %d more "
							  "conflicts like this "
							  "(y:Yes a:Yes to all "
							  "n:No q:No to all)"),
							filename,
							directory,
							--nfiles_exist),
						input_options);
				} else {
					c = v->confirm(
						strprintf::fmt(
							_("Overwrite `%s' in "
							  "`%s'? "
							  "There are no more "
							  "conflicts like this "
							  "(y:Yes a:Yes to all "
							  "n:No q:No to all)"),
							filename,
							directory),
						input_options);
				}
				if (!c) {
					break;
				}

				if (c == input_options.at(0)) {
					save_article(filepath, item);
				} else if (c == input_options.at(1)) {
					overwrite_all = true;
					save_article(filepath, item);
				} else if (c == input_options.at(2)) {
					continue;
				} else if (c == input_options.at(3)) {
					break;
				}
			} else {
				// Create file since it does not exist
				save_article(filepath, item);
			}
		}
	}
}

} // namespace newsboat

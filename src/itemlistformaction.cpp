#include <itemlistformaction.h>

#include <cinttypes>
#include <cstdio>
#include <cstring>
#include <langinfo.h>
#include <optional>
#include <sstream>
#include <string>
#include <sys/stat.h>


#include "config.h"
#include "controller.h"
#include "dbexception.h"
#include "fmtstrformatter.h"
#include "formaction.h"
#include "htmlrenderer.h"
#include "itemutils.h"
#include "logger.h"
#include "matcherexception.h"
#include "rssfeed.h"
#include "scopemeasure.h"
#include "strprintf.h"
#include "utils.h"
#include "view.h"

namespace newsboat {

ItemListFormAction::ItemListFormAction(View& vv,
	std::string formstr,
	Cache* cc,
	FilterContainer& f,
	ConfigContainer* cfg,
	RegexManager& r)
	: ListFormAction(vv, Dialog::ArticleList, formstr, "items", cfg, r)
	, old_itempos(-1)
	, filter_active(false)
	, pos(0)
	, set_filterpos(false)
	, filterpos(0)
	, rxman(r)
	, old_width(0)
	, invalidation_mode(InvalidationMode::NONE)
	, listfmt(&rxman, Dialog::ArticleList)
	, rsscache(cc)
	, filter_container(f)
{
	register_format_styles();
}

bool ItemListFormAction::process_operation(Operation op,
	const std::vector<std::string>& args,
	BindingType bindingType)
{
	bool quit = false;
	bool hardquit = false;

	/*
	 * most of the operations go like this:
	 *   - extract the current position
	 *   - if an item was selected, then fetch it and do something with it
	 */

	const unsigned int itempos = list.get_position();

	switch (op) {
	case OP_OPEN: {
		LOG(Level::INFO, "ItemListFormAction: opening item at pos `%u'", itempos);
		if (!visible_items.empty()) {
			// no need to mark item as read, the itemview already do
			// that
			old_itempos = itempos;
			v.push_itemview(feed,
				visible_items[itempos].first->guid());
			invalidate(itempos);
		} else {
			v.get_statusline().show_error(
				_("No item selected!")); // should not happen
		}
	}
	break;
	case OP_DELETE: {
		ScopeMeasure m1("OP_DELETE");
		if (!visible_items.empty()) {
			// mark as read
			v.get_ctrl().mark_article_read(
				visible_items[itempos].first->guid(), true);
			visible_items[itempos].first->set_unread(false);
			// mark as deleted
			visible_items[itempos].first->set_deleted(
				!visible_items[itempos].first->deleted());
			rsscache->mark_item_deleted(
				visible_items[itempos].first->guid(),
				visible_items[itempos].first->deleted());
			if (itempos < visible_items.size() - 1) {
				list.set_position(itempos + 1);
			}
			invalidate(itempos);
		} else {
			v.get_statusline().show_error(
				_("No item selected!")); // should not happen
		}
	}
	break;
	case OP_DELETE_ALL: {
		if (!cfg->get_configvalue_as_bool("confirm-delete-all-articles") ||
			v.confirm(_("Do you really want to delete all articles (y:Yes n:No)? "),
				_("yn")) == *_("y")) {
			ScopeMeasure m1("OP_DELETE_ALL");

			std::vector<std::string> item_guids;
			for (const auto& pair : visible_items) {
				const auto item = pair.first;
				item_guids.push_back(item->guid());
			}
			v.get_ctrl().mark_all_read(item_guids);

			for (const auto& pair : visible_items) {
				const auto item = pair.first;

				// mark as read
				item->set_unread(false);
				// mark as deleted
				item->set_deleted(true);
				rsscache->mark_item_deleted(item->guid(), true);
			}
			invalidate_list();
		}
	}
	break;
	case OP_PURGE_DELETED: {
		ScopeMeasure m1("OP_PURGE_DELETED");
		feed->purge_deleted_items();
		invalidate_list();
	}
	break;
	case OP_OPENBROWSER_AND_MARK: {
		const bool interactive = true;
		invalidate(itempos);
		if (!open_position_in_browser(itempos, interactive)) {
			return false;
		}

		auto item = visible_items[itempos].first;
		item->set_unread(false);
		v.get_ctrl().mark_article_read(item->guid(), true);
		if (cfg->get_configvalue_as_bool("openbrowser-and-mark-jumps-to-next-unread")) {
			std::vector<std::string> args;
			process_operation(OP_NEXTUNREAD, args);
		} else {
			if (itempos < visible_items.size() - 1) {
				list.set_position(itempos + 1);
			}
		}
	}
	break;
	case OP_OPENINBROWSER: {
		const bool interactive = true;
		invalidate(itempos);
		return open_position_in_browser(itempos, interactive);
	}
	break;
	case OP_OPENINBROWSER_NONINTERACTIVE: {
		const bool interactive = false;
		invalidate(itempos);
		return open_position_in_browser(itempos, interactive);
	}
	break;
	case OP_OPENALLUNREADINBROWSER: {
		if (feed) {
			LOG(Level::INFO,
				"ItemListFormAction: opening all unread items "
				"in "
				"browser");

			// We can't just `const auto exit_code = ...` here because this
			// triggers -Wmaybe-initialized in GCC 9 with -O2.
			std::optional<std::uint8_t> exit_code;
			exit_code = open_unread_items_in_browser(feed, false);

			if (!exit_code.has_value()) {
				v.get_statusline().show_error(_("Failed to spawn browser"));
				return false;
			} else if (*exit_code != 0) {
				v.get_statusline().show_error(strprintf::fmt(_("Browser returned error code %i"),
						*exit_code));
				return false;
			}
		}
	}
	break;
	case OP_OPENALLUNREADINBROWSER_AND_MARK: {
		if (feed) {
			LOG(Level::INFO,
				"ItemListFormAction: opening all unread items "
				"in "
				"browser and marking read");

			// We can't just `const auto exit_code = ...` here because this
			// triggers -Wmaybe-initialized in GCC 9 with -O2.
			std::optional<std::uint8_t> exit_code;
			exit_code = open_unread_items_in_browser(feed, true);

			if (!exit_code.has_value()) {
				v.get_statusline().show_error(_("Failed to spawn browser"));
				return false;
			} else if (*exit_code != 0) {
				v.get_statusline().show_error(strprintf::fmt(_("Browser returned error code %i"),
						*exit_code));
				return false;
			}
			invalidate_list();
		}
	}
	break;
	case OP_TOGGLEITEMREAD: {
		LOG(Level::INFO, "ItemListFormAction: toggling item read at pos `%u'", itempos);
		if (!visible_items.empty()) {
			try {
				const auto message_lifetime = v.get_statusline().show_message_until_finished(
						_("Toggling read flag for article..."));
				if (args.size() > 0) {
					if (args.front() == "read") {
						visible_items[itempos]
						.first->set_unread(
							false);
						v.get_ctrl().mark_article_read(
							visible_items[itempos]
							.first->guid(),
							true);
					} else if (args.front() == "unread") {
						visible_items[itempos]
						.first->set_unread(
							true);
						v.get_ctrl().mark_article_read(
							visible_items[itempos]
							.first->guid(),
							false);
					}
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
					v.get_ctrl().mark_article_read(
						visible_items[itempos]
						.first->guid(),
						unread);
				}
			} catch (const DbException& e) {
				v.get_statusline().show_error(strprintf::fmt(
						_("Error while toggling read flag: %s"),
						e.what()));
			}
			if (cfg->get_configvalue_as_bool(
					"toggleitemread-jumps-to-next-unread")) {
				std::vector<std::string> args;
				process_operation(OP_NEXTUNREAD, args);
			} else if (cfg->get_configvalue_as_bool(
					"toggleitemread-jumps-to-next")) {
				if (itempos < visible_items.size() - 1) {
					list.set_position(itempos + 1);
				}
			}
			invalidate(itempos);
		}
	}
	break;
	case OP_SHOWURLS:
		if (!visible_items.empty()) {
			if (itempos < visible_items.size()) {
				const auto urlviewer = cfg->get_configvalue_as_filepath("external-url-viewer");
				if (urlviewer == Filepath{}) {
					Links links;
					std::vector<std::pair<LineType, std::string>> lines;
					HtmlRenderer rnd;
					std::string baseurl =
						visible_items[itempos].first->get_base() != ""
						? visible_items[itempos].first->get_base()
						: visible_items[itempos].first->feedurl();
					rnd.render(
						utils::utf8_to_locale(visible_items[itempos].first->description().text),
						lines,
						links,
						baseurl);
					if (!links.empty()) {
						v.push_urlview(links, feed);
					} else {
						v.get_statusline().show_error(
							_("URL list empty."));
					}
				} else {
					qna_responses.clear();
					qna_responses.push_back(urlviewer.to_locale_string());
					this->finished_qna(QnaFinishAction::PipeItemIntoProgram);
				}
			}
		} else {
			v.get_statusline().show_error(
				_("No item selected!")); // should not happen
		}
		break;
	case OP_BOOKMARK: {
		LOG(Level::INFO, "ItemListFormAction: bookmarking item at pos `%u'", itempos);
		if (!visible_items.empty()) {
			if (itempos < visible_items.size()) {
				switch (bindingType) {
				case BindingType::Bind:
					if (args.empty()) {
						this->start_bookmark_qna(
							visible_items[itempos].first->title(),
							visible_items[itempos].first->link(),
							feed->title());
					} else {
						qna_responses = {
							visible_items[itempos].first->link(),
							utils::utf8_to_locale(visible_items[itempos].first->title()),
							args.front(),
							feed->title(),
						};
						this->finished_qna(QnaFinishAction::Bookmark);
					}
					break;
				case BindingType::Macro:
					qna_responses.clear();
					qna_responses.push_back(
						visible_items[itempos]
						.first->link());
					qna_responses.push_back(utils::utf8_to_locale(
							visible_items[itempos].first->title()));
					qna_responses.push_back(args.size() > 0
						? args.front()
						: "");
					qna_responses.push_back(feed->title());
					this->finished_qna(QnaFinishAction::Bookmark);
					break;
				case BindingType::BindKey:
					this->start_bookmark_qna(
						visible_items[itempos].first->title(),
						visible_items[itempos].first->link(),
						feed->title());
					break;
				}
			}
		} else {
			v.get_statusline().show_error(
				_("No item selected!")); // should not happen
		}
	}
	break;
	case OP_EDITFLAGS: {
		if (!visible_items.empty()) {
			if (itempos < visible_items.size()) {
				switch (bindingType) {
				case BindingType::Bind:
					if (args.empty()) {
						std::vector<QnaPair> qna {
							QnaPair(_("Flags: "), visible_items[itempos].first->flags())
						};
						this->start_qna(qna, QnaFinishAction::UpdateFlags);
					} else {
						qna_responses = {args.front()};
						finished_qna(QnaFinishAction::UpdateFlags);
					}
					break;
				case BindingType::Macro:
					if (args.size() > 0) {
						qna_responses.clear();
						qna_responses.push_back(
							args.front());
						finished_qna(
							QnaFinishAction::UpdateFlags);
					}
					break;
				case BindingType::BindKey:
					std::vector<QnaPair> qna;
					qna.push_back(QnaPair(_("Flags: "),
							visible_items[itempos].first->flags()));
					this->start_qna(qna, QnaFinishAction::UpdateFlags);
					break;
				}
			}
		} else {
			v.get_statusline().show_error(
				_("No item selected!")); // should not happen
		}
	}
	break;
	case OP_SAVE: {
		LOG(Level::INFO, "ItemListFormAction: saving item at pos `%u'", itempos);
		if (!visible_items.empty()) {
			std::shared_ptr<RssItem> item = visible_items[itempos].first;
			std::optional<Filepath> filename;
			switch (bindingType) {
			case BindingType::Bind:
				if (args.empty()) {
					const auto title = utils::utf8_to_locale(item->title());
					const auto suggestion = v.get_filename_suggestion(title);
					filename = v.run_filebrowser(suggestion);
				} else {
					filename = Filepath::from_locale_string(args.front());
				}
				break;
			case BindingType::Macro:
				if (args.size() > 0) {
					filename = Filepath::from_locale_string(args.front());
				}
				break;
			case BindingType::BindKey:
				const auto title = utils::utf8_to_locale(item->title());
				const auto suggestion = v.get_filename_suggestion(title);
				filename = v.run_filebrowser(suggestion);
				break;
			}
			save_article(filename, item);
		} else {
			v.get_statusline().show_error(_("Error: no item selected!"));
		}
	}
	break;
	case OP_SAVEALL:
		handle_op_saveall();
		break;
	case OP_HELP:
		v.push_help();
		break;
	case OP_RELOAD:
		LOG(Level::INFO, "ItemListFormAction: reloading current feed");
		v.get_ctrl().get_reloader()->reload(pos);
		invalidate_list();
		break;
	case OP_QUIT:
		LOG(Level::INFO, "ItemListFormAction: quitting");
		v.feedlist_mark_pos_if_visible(pos);
		feed->purge_deleted_items();
		feed->unload();
		quit = true;
		break;
	case OP_HARDQUIT:
		LOG(Level::INFO, "ItemListFormAction: hard quitting");
		v.feedlist_mark_pos_if_visible(pos);
		feed->purge_deleted_items();
		hardquit = true;
		break;
	case OP_NEXTUNREAD:
		LOG(Level::INFO,
			"ItemListFormAction: jumping to next unread item");
		if (!jump_to_next_unread_item(false)) {
			if (!v.get_next_unread(*this)) {
				v.get_statusline().show_error(_("No unread items."));
			}
		}
		break;
	case OP_PREVUNREAD:
		LOG(Level::INFO,
			"ItemListFormAction: jumping to previous unread item");
		if (!jump_to_previous_unread_item(false)) {
			if (!v.get_previous_unread(*this)) {
				v.get_statusline().show_error(_("No unread items."));
			}
		}
		break;
	case OP_NEXT:
		LOG(Level::INFO, "ItemListFormAction: jumping to next item");
		if (!jump_to_next_item(false)) {
			if (!v.get_next(*this)) {
				v.get_statusline().show_error(_("Already on last item."));
			}
		}
		break;
	case OP_PREV:
		LOG(Level::INFO,
			"ItemListFormAction: jumping to previous item");
		if (!jump_to_previous_item(false)) {
			if (!v.get_previous(*this)) {
				v.get_statusline().show_error(_("Already on first item."));
			}
		}
		break;
	case OP_RANDOMUNREAD:
		if (!jump_to_random_unread_item()) {
			if (!v.get_random_unread(*this)) {
				v.get_statusline().show_error(_("No unread items."));
			}
		}
		break;
	case OP_NEXTUNREADFEED:
		if (!v.get_next_unread_feed(*this)) {
			v.get_statusline().show_error(_("No unread feeds."));
		}
		break;
	case OP_PREVUNREADFEED:
		if (!v.get_prev_unread_feed(*this)) {
			v.get_statusline().show_error(_("No unread feeds."));
		}
		break;
	case OP_NEXTFEED:
		if (!v.get_next_feed(*this)) {
			v.get_statusline().show_error(_("Already on last feed."));
		}
		break;
	case OP_PREVFEED:
		if (!v.get_prev_feed(*this)) {
			v.get_statusline().show_error(_("Already on first feed."));
		}
		break;
	case OP_MARKFEEDREAD:
		if (!cfg->get_configvalue_as_bool(
				"confirm-mark-feed-read") ||
			v.confirm(_("Do you really want to mark this feed as read (y:Yes n:No)? "),
				_("yn")) == *_("y")) {
			LOG(Level::INFO, "ItemListFormAction: marking feed read");
			try {
				const auto message_lifetime = v.get_statusline().show_message_until_finished(
						_("Marking feed read..."));

				std::vector<std::string> guids;
				for (const auto& item : visible_items) {
					const std::string guid = item.first->guid();
					guids.push_back(guid);
				}
				rsscache->mark_items_read_by_guid(guids);

				if (filter_active) {
					// We're only viewing a subset of items, so mark them off one by one.
					v.get_ctrl().mark_all_read(guids);
				} else {
					v.get_ctrl().mark_all_read(pos);
				}

				if (visible_items.size() > 0) {
					std::lock_guard<std::mutex> lock(feed->item_mutex);
					bool notify = visible_items[0].first->feedurl() != feed->rssurl();
					for (const auto& item : visible_items) {
						item.first->set_unread_nowrite_notify(false, notify);
					}
				}
				if (cfg->get_configvalue_as_bool("markfeedread-jumps-to-next-unread")) {
					std::vector<std::string> args;
					process_operation(OP_NEXTUNREAD, args);
				} else { // reposition to first/last item
					std::string sortorder =
						cfg->get_configvalue("article-sort-order");

					if (sortorder == "date-desc") {
						LOG(Level::DEBUG,
							"ItemListFormAction:: "
							"reset itempos to last");
						list.set_position(visible_items.size() - 1);
					}
					if (sortorder == "date-asc") {
						LOG(Level::DEBUG,
							"ItemListFormAction:: "
							"reset itempos to first");
						list.set_position(0);
					}
				}
				invalidate_list();
			} catch (const DbException& e) {
				v.get_statusline().show_error(strprintf::fmt(
						_("Error: couldn't mark feed read: %s"),
						e.what()));
			}
		}
		break;
	case OP_MARKALLABOVEASREAD: {
		LOG(Level::INFO,
			"ItemListFormAction: marking all above as read");
		const auto message_lifetime = v.get_statusline().show_message_until_finished(
				_("Marking all above as read..."));
		if (itempos < visible_items.size()) {
			for (unsigned int i = 0; i < itempos; ++i) {
				if (visible_items[i].first->unread()) {
					visible_items[i].first->set_unread(
						false);
					v.get_ctrl().mark_article_read(
						visible_items[i].first->guid(),
						true);
				}
			}
			if (!cfg->get_configvalue_as_bool(
					"show-read-articles")) {
				list.set_position(0);
			}
			invalidate_list();
		}
		break;
	}
	case OP_TOGGLESHOWREAD:
		LOG(Level::DEBUG,
			"ItemListFormAction: toggling show-read-articles");
		if (cfg->get_configvalue_as_bool("show-read-articles")) {
			cfg->set_configvalue("show-read-articles", "no");
		} else {
			cfg->set_configvalue("show-read-articles", "yes");
		}
		save_filterpos();
		invalidate_list();
		break;
	case OP_PIPE_TO:
		if (visible_items.size() != 0) {
			std::vector<QnaPair> qna;
			switch (bindingType) {
			case BindingType::Bind:
				if (args.empty()) {
					qna.push_back(QnaPair(_("Pipe article to command: "), ""));
					this->start_qna(qna, QnaFinishAction::PipeItemIntoProgram, &cmdlinehistory);
				} else {
					qna_responses = { args.front() };
					finished_qna(QnaFinishAction::PipeItemIntoProgram);
				}
				break;
			case BindingType::Macro:
				if (args.size() > 0) {
					qna_responses.clear();
					qna_responses.push_back(args.front());
					finished_qna(QnaFinishAction::PipeItemIntoProgram);
				}
				break;
			case BindingType::BindKey:
				qna.push_back(QnaPair(
						_("Pipe article to command: "), ""));
				this->start_qna(
					qna, QnaFinishAction::PipeItemIntoProgram, &cmdlinehistory);
				break;
			}
		} else {
			v.get_statusline().show_error(_("No item selected!"));
		}
		break;
	case OP_SEARCH: {
		std::vector<QnaPair> qna;
		switch (bindingType) {
		case BindingType::Bind:
			if (args.empty()) {
				qna.push_back(QnaPair(_("Search for: "), ""));
				this->start_qna(qna, QnaFinishAction::Search, &searchhistory);
			} else {
				qna_responses = { args.front() };
				finished_qna(QnaFinishAction::Search);
			}
			break;
		case BindingType::Macro:
			if (args.size() > 0) {
				qna_responses.clear();
				qna_responses.push_back(args.front());
				finished_qna(QnaFinishAction::Search);
			}
			break;
		case BindingType::BindKey:
			qna.push_back(QnaPair(_("Search for: "), ""));
			this->start_qna(
				qna, QnaFinishAction::Search, &searchhistory);
			break;
		}
	}
	break;
	case OP_GOTO_TITLE:
		switch (bindingType) {
		case BindingType::Bind:
			if (args.empty()) {
				std::vector<QnaPair> qna {
					QnaPair(_("Title: "), ""),
				};
				this->start_qna(qna, QnaFinishAction::GotoTitle);
			} else {
				qna_responses = {args[0]};
				finished_qna(QnaFinishAction::GotoTitle);
			}
			break;
		case BindingType::Macro:
			if (args.size() >= 1) {
				qna_responses = {args[0]};
				finished_qna(QnaFinishAction::GotoTitle);
			}
			break;
		case BindingType::BindKey:
			std::vector<QnaPair> qna;
			qna.push_back(QnaPair(_("Title: "), ""));
			this->start_qna(qna, QnaFinishAction::GotoTitle);
			break;
		}
		break;
	case OP_EDIT_URLS:
		v.get_ctrl().edit_urls_file();
		break;
	case OP_SELECTFILTER:
		if (filter_container.size() > 0) {
			std::string newfilter;
			if (args.size() > 0) {
				const std::string filter_name = args.front();
				const auto filter = filter_container.get_filter(filter_name);

				if (filter.has_value()) {
					apply_filter(filter.value());
				} else {
					v.get_statusline().show_error(strprintf::fmt(_("No filter found with name `%s'."),
							filter_name));
				}
			} else {
				const std::string filter_text = v.select_filter(filter_container.get_filters());
				apply_filter(filter_text);
			}
		} else {
			v.get_statusline().show_error(_("No filters defined."));
		}

		break;
	case OP_SETFILTER:
		switch (bindingType) {
		case BindingType::Bind:
			if (args.empty()) {
				std::vector<QnaPair> qna {
					QnaPair(_("Filter: "), ""),
				};
				this->start_qna(qna, QnaFinishAction::SetFilter, &filterhistory);
			} else {
				qna_responses = { args.front() };
				this->finished_qna(QnaFinishAction::SetFilter);
			}
			break;
		case BindingType::Macro:
			if (args.size() > 0) {
				qna_responses.clear();
				qna_responses.push_back(args.front());
				this->finished_qna(QnaFinishAction::SetFilter);
			}
			break;
		case BindingType::BindKey:
			std::vector<QnaPair> qna;
			qna.push_back(QnaPair(_("Filter: "), ""));
			this->start_qna(
				qna, QnaFinishAction::SetFilter, &filterhistory);
			break;
		}
		break;
	case OP_CLEARFILTER:
		filter_active = false;
		invalidate_list();
		save_filterpos();
		break;
	case OP_SORT: {
		// i18n: This string is related to the letters in parentheses in the
		// "Sort by (d)ate/..." and "Reverse Sort by (d)ate/..."
		// messages
		std::string input_options = _("dtfalgr");
		char c = v.confirm(
				_("Sort by "
					"(d)ate/(t)itle/(f)lags/(a)uthor/(l)ink/(g)uid/(r)andom?"),
				input_options);
		if (!c) {
			break;
		}

		// Check that the number of translated answers is the same as the
		// number of answers we expect to handle. If it doesn't, just give up.
		// That'll prevent this function from sorting anything, so users will
		// complain, and we'll ask them to update the translation. A bit lame,
		// but it's better than mishandling the answer.
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
	}
	break;
	case OP_REVSORT: {
		std::string input_options = _("dtfalgr");
		char c = v.confirm(
				_("Reverse Sort by "
					"(d)ate/(t)itle/(f)lags/(a)uthor/(l)ink/(g)uid/(r)andom?"),
				input_options);
		if (!c) {
			break;
		}

		// Check that the number of translated answers is the same as the
		// number of answers we expect to handle. If it doesn't, just give up.
		// That'll prevent this function from sorting anything, so users will
		// complain, and we'll ask them to update the translation. A bit lame,
		// but it's better than mishandling the answer.
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
	}
	break;
	case OP_ENQUEUE:
		if (!visible_items.empty() && itempos < visible_items.size()) {
			const auto item = visible_items[itempos].first;
			return enqueue_item_enclosure(*item, *feed, v, *rsscache);
		} else {
			v.get_statusline().show_error(_("No item selected!"));
			return false;
		}
		break;
	case OP_ARTICLEFEED: {
		auto feeds = v.get_ctrl().get_feedcontainer()->get_all_feeds();
		size_t pos;
		auto article_feed = visible_items[itempos].first->get_feedptr();
		for (pos = 0; pos < feeds.size(); pos++) {
			if (feeds[pos] == article_feed) {
				break;
			}
		}
		if (pos != feeds.size()) {
			v.push_itemlist(pos);
		}
	}
	break;
	default:
		return ListFormAction::process_operation(op, args, bindingType);
	}
	if (hardquit) {
		while (v.formaction_stack_size() > 0) {
			v.pop_current_formaction();
		}
	} else if (quit) {
		v.pop_current_formaction();
	}
	return true;
}

bool ItemListFormAction::open_position_in_browser(
	unsigned int pos, bool interactive) const
{
	LOG(Level::INFO,
		"ItemListFormAction: opening item at pos `%u', interactive: %s",
		pos,
		interactive ? "true" : "false");
	if (!visible_items.empty() && pos < visible_items.size()) {
		const auto item = visible_items[pos].first;
		const auto link = item->link();
		const auto feedurl = item->feedurl();
		const auto exit_code = v.open_in_browser(link, feedurl, "article", item->title(),
				interactive);
		if (!exit_code.has_value()) {
			v.get_statusline().show_error(_("Failed to spawn browser"));
			return false;
		} else if (*exit_code != 0) {
			v.get_statusline().show_error(strprintf::fmt(_("Browser returned error code %i"),
					*exit_code));
			return false;
		}
		return true;
	} else {
		v.get_statusline().show_error(_("No item selected!"));
		return false;
	}
}

void ItemListFormAction::finished_qna(QnaFinishAction op)
{
	FormAction::finished_qna(op); // important!

	switch (op) {
	case QnaFinishAction::SetFilter:
		qna_end_setfilter();
		break;

	case QnaFinishAction::UpdateFlags:
		qna_end_editflags();
		break;

	case QnaFinishAction::Search:
		qna_start_search();
		break;

	case QnaFinishAction::GotoTitle:
		goto_item(qna_responses[0]);
		break;

	case QnaFinishAction::PipeItemIntoProgram: {
		if (!visible_items.empty()) {
			unsigned int itempos = list.get_position();
			std::string cmd = qna_responses[0];
			std::ostringstream ostr;
			v.get_ctrl().write_item(
				*visible_items[itempos].first, ostr);
			v.push_empty_formaction();
			Stfl::reset();
			FILE* f = popen(cmd.c_str(), "w");
			if (f) {
				std::string data = ostr.str();
				fwrite(data.c_str(), data.length(), 1, f);
				pclose(f);
			}
			v.drop_queued_input();
			v.pop_current_formaction();
		}
	}
	break;

	default:
		break;
	}
}

void ItemListFormAction::qna_end_setfilter()
{
	std::string filtertext = qna_responses[0];
	apply_filter(filtertext);
}

void ItemListFormAction::qna_end_editflags()
{
	if (visible_items.empty()) {
		v.get_statusline().show_error(_("No item selected!")); // should not happen
		return;
	}

	const unsigned int itempos = list.get_position();
	if (itempos < visible_items.size()) {
		visible_items[itempos].first->set_flags(qna_responses[0]);
		v.get_ctrl().update_flags(visible_items[itempos].first);
		v.get_statusline().show_message(_("Flags updated."));
		LOG(Level::DEBUG,
			"ItemListFormAction::finished_qna: updated flags");
		invalidate(itempos);
	}
}

void ItemListFormAction::qna_start_search()
{
	const std::string searchphrase = qna_responses[0];
	if (searchphrase.length() == 0) {
		return;
	}

	searchhistory.add_line(searchphrase);
	std::vector<std::shared_ptr<RssItem>> items;
	try {
		const auto message_lifetime = v.get_statusline().show_message_until_finished(
				_("Searching..."));
		const auto utf8searchphrase = utils::locale_to_utf8(searchphrase);
		items = v.get_ctrl().search_for_items(
				utf8searchphrase, feed);
	} catch (const DbException& e) {
		v.get_statusline().show_error(
			strprintf::fmt(_("Error while searching for `%s': %s"),
				searchphrase,
				e.what()));
		return;
	}

	if (items.empty()) {
		v.get_statusline().show_error(_("No results."));
		return;
	}

	std::shared_ptr<RssFeed> search_dummy_feed(new RssFeed(rsscache, ""));
	search_dummy_feed->set_search_feed(true);
	search_dummy_feed->add_items(items);
	v.push_searchresult(search_dummy_feed, searchphrase);
}

void ItemListFormAction::do_update_visible_items()
{
	if (invalidation_mode != InvalidationMode::COMPLETE) {
		return;
	}

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
			(!filter_active || matcher.matches(item.get()))) {
			new_visible_items.push_back(ItemPtrPosPair(item, i));
		}
		i++;
	}

	LOG(Level::DEBUG,
		"ItemListFormAction::do_update_visible_items: size = %" PRIu64,
		static_cast<uint64_t>(visible_items.size()));

	const unsigned int pos = list.get_position();
	unsigned int new_pos = pos;
	for (unsigned int old_i = 0, new_i = 0;
		old_i < visible_items.size() &&
		new_i < new_visible_items.size() &&
		old_i < pos; ) {
		int cmp = visible_items[old_i].second < new_visible_items[new_i].second ? -1 :
			(visible_items[old_i].second == new_visible_items[new_i].second ? 0 : 1);
		if (cmp < 0) {
			if (new_pos > 0) {
				new_pos--;
			}
			old_i++;
		} else if (cmp == 0) {
			old_i++;
			new_i++;
		} else {
			new_pos++;
			new_i++;
		}
	}
	list.set_position(new_pos);

	visible_items = new_visible_items;
}

void ItemListFormAction::draw_items()
{
	auto datetime_format = cfg->get_configvalue("datetime-format");
	auto itemlist_format =
		cfg->get_configvalue("articlelist-format");

	auto render_line = [this, itemlist_format, datetime_format](std::uint32_t line,
	std::uint32_t width) -> StflRichText {
		if (line >= visible_items.size())
		{
			return StflRichText::from_plaintext("ERROR");
		}
		auto& item = visible_items[line];
		return item2formatted_line(item, width, itemlist_format, datetime_format);
	};
	list.invalidate_list_content(visible_items.size(), render_line);

	invalidated_itempos.clear();
	invalidation_mode = InvalidationMode::NONE;
}

void ItemListFormAction::prepare()
{
	set_keymap_hints();

	std::lock_guard<std::mutex> mtx(redraw_mtx);

	const auto sort_strategy = cfg->get_article_sort_strategy();
	if (!old_sort_strategy || sort_strategy != *old_sort_strategy) {
		feed->sort(sort_strategy);
		old_sort_strategy = sort_strategy;
		invalidate_list();
	}

	try {
		do_update_visible_items();
	} catch (MatcherException& e) {
		v.get_statusline().show_error(strprintf::fmt(
				_("Error: applying the filter failed: %s"), e.what()));
		return;
	}

	if (cfg->get_configvalue_as_bool("mark-as-read-on-hover")) {
		if (!visible_items.empty()) {
			const unsigned int itempos = list.get_position();
			if (visible_items[itempos].first->unread()) {
				visible_items[itempos].first->set_unread(false);
				v.get_ctrl().mark_article_read(
					visible_items[itempos].first->guid(),
					true);
				invalidate(itempos);
			}
		}
	}

	const unsigned int width = list.get_width();

	if (do_redraw || old_width != width) {
		invalidate_list();
		old_width = width;
		do_redraw = false;
	}

	if (invalidation_mode == InvalidationMode::NONE) {
		return;
	}

	draw_items();

	set_head(feed->title(),
		feed->unread_item_count(),
		feed->total_item_count(),
		feed->rssurl());

	prepare_set_filterpos();
}

StflRichText ItemListFormAction::item2formatted_line(const ItemPtrPosPair& item,
	const unsigned int width,
	const std::string& itemlist_format,
	const std::string& datetime_format) const
{
	FmtStrFormatter fmt;
	fmt.register_fmt('i', strprintf::fmt("%u", item.second + 1));
	fmt.register_fmt('f', gen_flags(item.first));
	fmt.register_fmt('n', item.first->unread() ? "N" : " ");
	fmt.register_fmt('d', item.first->deleted() ? "D" : " ");
	fmt.register_fmt('F', item.first->flags());
	fmt.register_fmt('e', item.first->enclosure_url());

	using namespace std::chrono;
	const auto article_time_point = system_clock::from_time_t(
			item.first->pubDate_timestamp());
	using days = duration<int, std::ratio<86400>>;
	const auto article_age = duration_cast<days>(
			system_clock::now() - article_time_point).count();
	const std::string new_datetime_format = utils::replace_all(
			datetime_format, "%L", strprintf::fmt(
				ngettext("1 day ago", "%u days ago", article_age), article_age));
	fmt.register_fmt('D', utils::mt_strf_localtime(new_datetime_format,
			item.first->pubDate_timestamp()));

	if (feed->rssurl() != item.first->feedurl() &&
		item.first->get_feedptr() != nullptr) {
		auto feedtitle = item.first->get_feedptr()->title();
		utils::remove_soft_hyphens(feedtitle);
		fmt.register_fmt('T', feedtitle);
	}

	auto itemtitle = utils::utf8_to_locale(item.first->title());
	utils::remove_soft_hyphens(itemtitle);
	fmt.register_fmt('t', itemtitle);

	auto itemauthor = utils::utf8_to_locale(item.first->author());
	utils::remove_soft_hyphens(itemauthor);
	fmt.register_fmt('a', itemauthor);

	fmt.register_fmt('L', item.first->length());

	const auto formattedLine = fmt.do_format(itemlist_format, width);
	auto stflFormattedLine = StflRichText::from_plaintext(formattedLine);

	if (item.first->unread()) {
		stflFormattedLine.apply_style_tag("<unread>", 0, formattedLine.length());
	}

	const int id = rxman.article_matches(item.first.get());
	if (id != -1) {
		const auto tag = strprintf::fmt("<%d>", id);
		stflFormattedLine.apply_style_tag(tag, 0, formattedLine.length());
	}

	return stflFormattedLine;
}

void ItemListFormAction::goto_item(const std::string& title)
{
	if (visible_items.empty()) {
		return;
	}

	const unsigned int curpos = list.get_position();
	for (unsigned int i = curpos + 1; i < visible_items.size(); ++i) {
		if (strcasestr(visible_items[i].first->title().c_str(),
				title.c_str()) != nullptr) {
			list.set_position(i);
			return;
		}
	}
	for (unsigned int i = 0; i <= curpos; ++i) {
		if (strcasestr(visible_items[i].first->title().c_str(),
				title.c_str()) != nullptr) {
			list.set_position(i);
			return;
		}
	}
}

void ItemListFormAction::init()
{
	recalculate_widget_dimensions();
	list.set_position(0);
	set_status("");
	invalidate_list();
	do_update_visible_items();
	draw_items();
	if (cfg->get_configvalue_as_bool("goto-first-unread")) {
		jump_to_next_unread_item(true);
	}

	// This is a hack to make `prepare()` do all the work it's required to do
	// on the first run. Yes, we have a call to `invalidate_list() ` just a few
	// lines prior, but `draw_items()` above resets the mode back to `NONE`,
	// leading to https://github.com/newsboat/newsboat/issues/1385
	invalidate_list();
}

FmtStrFormatter ItemListFormAction::setup_head_formatter(const std::string& s,
	unsigned int unread,
	unsigned int total,
	const std::string& url)
{
	FmtStrFormatter fmt;

	fmt.register_fmt('N', PROGRAM_NAME);
	fmt.register_fmt('V', utils::program_version());

	fmt.register_fmt('u', std::to_string(unread));
	fmt.register_fmt('t', std::to_string(total));

	auto feedtitle = s;
	utils::remove_soft_hyphens(feedtitle);
	fmt.register_fmt('T', feedtitle);

	fmt.register_fmt('U', utils::censor_url(url));

	fmt.register_fmt('F', filter_active ? matcher.get_expression() : "");

	return fmt;
}

void ItemListFormAction::set_head(const std::string& s,
	unsigned int unread,
	unsigned int total,
	const std::string& url)
{
	FmtStrFormatter fmt = setup_head_formatter(s, unread, total, url);

	const unsigned int width = utils::to_u(f.get("title:w"));
	set_title(fmt.do_format(
			cfg->get_configvalue("articlelist-title-format"),
			width));
}

bool ItemListFormAction::jump_to_previous_unread_item(bool start_with_last)
{
	const int itempos = list.get_position();
	for (int i = (start_with_last ? itempos : (itempos - 1)); i >= 0; --i) {
		LOG(Level::DEBUG,
			"ItemListFormAction::jump_to_previous_unread_item: "
			"visible_items[%u] unread = %s",
			i,
			visible_items[i].first->unread() ? "true" : "false");
		if (visible_items[i].first->unread()) {
			list.set_position(i);
			return true;
		}
	}
	for (int i = visible_items.size() - 1; i >= itempos; --i) {
		if (visible_items[i].first->unread()) {
			list.set_position(i);
			return true;
		}
	}
	return false;
}

bool ItemListFormAction::jump_to_random_unread_item()
{
	std::vector<unsigned int> unread_indexes;
	for (unsigned int i = 0; i < visible_items.size(); ++i) {
		if (visible_items[i].first->unread()) {
			unread_indexes.push_back(i);
		}
	}
	if (!unread_indexes.empty()) {
		const unsigned int selected = utils::get_random_value(unread_indexes.size());
		const unsigned int pos = unread_indexes[selected];
		list.set_position(pos);
		return true;
	}
	return false;
}

bool ItemListFormAction::jump_to_next_unread_item(bool start_with_first)
{
	const unsigned int itempos = list.get_position();
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
			list.set_position(i);
			return true;
		}
	}
	for (unsigned int i = 0; i <= itempos && i < visible_items.size();
		++i) {
		LOG(Level::DEBUG,
			"ItemListFormAction::jump_to_next_unread_item: i = %u",
			i);
		if (visible_items[i].first->unread()) {
			list.set_position(i);
			return true;
		}
	}
	return false;
}

bool ItemListFormAction::jump_to_previous_item(bool start_with_last)
{
	const unsigned int itempos = list.get_position();

	int i = (start_with_last ? itempos : (itempos - 1));
	if (i >= 0) {
		LOG(Level::DEBUG,
			"ItemListFormAction::jump_to_previous_item: "
			"visible_items[%u]",
			i);
		list.set_position(i);
		return true;
	}
	return false;
}

bool ItemListFormAction::jump_to_next_item(bool start_with_first)
{
	const unsigned int itempos = list.get_position();
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
		list.set_position(i);
		return true;
	}
	return false;
}

std::string ItemListFormAction::get_guid()
{
	const unsigned int itempos = list.get_position();
	return visible_items[itempos].first->guid();
}

std::vector<KeyMapHintEntry> ItemListFormAction::get_keymap_hint() const
{
	std::vector<KeyMapHintEntry> hints;
	if (filter_active) {
		hints.push_back({OP_CLEARFILTER, _("Clear filter")});
	}
	hints.push_back({OP_QUIT, _("Quit")});
	hints.push_back({OP_OPEN, _("Open")});
	hints.push_back({OP_SAVE, _("Save")});
	hints.push_back({OP_RELOAD, _("Reload")});
	hints.push_back({OP_NEXTUNREAD, _("Next Unread")});
	hints.push_back({OP_MARKFEEDREAD, _("Mark All Read")});
	hints.push_back({OP_SEARCH, _("Search")});
	hints.push_back({OP_HELP, _("Help")});
	return hints;
}

void ItemListFormAction::handle_cmdline_num(unsigned int idx)
{
	if (idx > 0 &&
		idx <= visible_items[visible_items.size() - 1].second + 1) {
		int i = get_pos(idx - 1);
		if (i == -1) {
			v.get_statusline().show_error(_("Position not visible!"));
		} else {
			list.set_position(i);
		}
	} else {
		v.get_statusline().show_error(_("Invalid position!"));
	}
}

void ItemListFormAction::handle_cmdline(const std::string& cmd)
{
	unsigned int idx = 0;
	if (1 == sscanf(cmd.c_str(), "%u", &idx)) {
		handle_cmdline_num(idx);
	} else {
		const auto command = FormAction::parse_command(cmd);
		switch (command.type) {
		case CommandType::SAVE:
			if (!command.args.empty()) {
				handle_save(command.args);
			}
			break;
		default:
			FormAction::handle_parsed_command(command);
		}
	}
}

int ItemListFormAction::get_pos(unsigned int realidx)
{
	for (unsigned int i = 0; i < visible_items.size(); ++i) {
		if (visible_items[i].second == realidx) {
			return i;
		}
	}
	return -1;
}

void ItemListFormAction::restore_selected_position()
{
	// If the old position was set and it is less than the current itempos, use
	// it for the feed's itempos.
	// This corrects a problem which occurs when you open an article, move to
	// the next article (one or more times), and then exit to the itemlist. If
	// `"show-read-articles" == false`, the itempos would be "wrong" without
	// this fix.
	const unsigned int itempos = list.get_position();
	if ((old_itempos != -1) && itempos > (unsigned int)old_itempos &&
		!cfg->get_configvalue_as_bool("show-read-articles")) {
		list.set_position(old_itempos);
		old_itempos = -1; // Reset
	}

}

void ItemListFormAction::save_article(const std::optional<Filepath>& filename,
	std::shared_ptr<RssItem> item)
{
	if (!filename.has_value()) {
		v.get_statusline().show_error(_("Aborted saving."));
	} else {
		try {
			v.get_ctrl().write_item(*item, filename.value());
			v.get_statusline().show_message(strprintf::fmt(
					_("Saved article to %s"), filename.value()));
		} catch (...) {
			v.get_statusline().show_error(strprintf::fmt(
					_("Error: couldn't save article to %s"),
					filename.value()));
		}
	}
}

void ItemListFormAction::handle_save(const std::vector<std::string>& cmd_args)
{
	if (cmd_args.size() < 1) {
		v.get_statusline().show_error(_("Error: no filename provided"));
		return;
	}
	if (visible_items.empty()) {
		v.get_statusline().show_error(_("Error: no item selected!"));
		return;
	}
	const auto path = Filepath::from_locale_string(cmd_args.front());
	const Filepath filename = utils::resolve_tilde(path);
	const unsigned int itempos = list.get_position();
	LOG(Level::INFO,
		"ItemListFormAction::handle_cmdline: saving item at pos `%u' to `%s'",
		itempos,
		filename);
	save_article(filename, visible_items[itempos].first);
}

void ItemListFormAction::save_filterpos()
{
	const unsigned int i = list.get_position();
	if (i < visible_items.size()) {
		filterpos = visible_items[i].second;
		set_filterpos = true;
	}
}

void ItemListFormAction::register_format_styles()
{
	const std::string attrstr = rxman.get_attrs_stfl_string(Dialog::ArticleList, true);
	const std::string textview = strprintf::fmt(
			"{!list[items] .expand:vh style_normal[listnormal]: "
			"style_focus[listfocus]: "
			"pos[items_pos]:0 offset[items_offset]:0 %s richtext:1}",
			attrstr);
	list.stfl_replace_list(textview);
}

std::string ItemListFormAction::gen_flags(std::shared_ptr<RssItem> item) const
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

void ItemListFormAction::prepare_set_filterpos()
{
	if (set_filterpos) {
		set_filterpos = false;
		unsigned int i = 0;
		for (const auto& item : visible_items) {
			if (item.second == filterpos) {
				list.set_position(i);
				return;
			}
			i++;
		}
		list.set_position(0);
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
	invalidate_list();
	do_update_visible_items();
}

std::string ItemListFormAction::title()
{
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

void ItemListFormAction::handle_op_saveall()
{
	LOG(Level::INFO,
		"ItemListFormAction: saving all items");

	if (visible_items.empty()) {
		return;
	}

	const std::optional<Filepath> directory = v.run_dirbrowser();
	if (!directory.has_value()) {
		return;
	}

	std::vector<Filepath> filenames;
	for (const auto& item : visible_items) {
		filenames.emplace_back(v.get_filename_suggestion(item.first->title()));
	}

	const auto unique_filenames = std::set<Filepath>(
			std::begin(filenames),
			std::end(filenames));

	int nfiles_exist = filenames.size() - unique_filenames.size();
	for (const auto& filename : unique_filenames) {
		const auto filepath = directory.value().join(filename);
		struct stat sbuf;
		if (::stat(filepath.to_locale_string().c_str(), &sbuf) != -1) {
			nfiles_exist++;
		}
	}

	// Check that the number of translated answers is the same as the
	// number of answers we expect to handle. If it doesn't, just give up.
	// That'll prevent this function from saving anything, so users will
	// complain, and we'll ask them to update the translation. A bit lame,
	// but it's better than mishandling the answer.
	const std::string input_options = _("yanq");
	const auto n_options = ((std::string) "yanq").length();
	if (input_options.length() < n_options) {
		return;
	}

	bool overwrite_all = false;
	for (size_t item_idx = 0; item_idx < filenames.size(); ++item_idx) {
		const auto filename = filenames[item_idx];
		const auto filepath = directory.value().join(filename);
		auto item = visible_items[item_idx].first;

		struct stat sbuf;
		if (::stat(filepath.to_locale_string().c_str(), &sbuf) != -1) {
			if (overwrite_all) {
				save_article(filepath, item);
				continue;
			}

			char c;
			if (nfiles_exist > 1) {
				c = v.confirm(strprintf::fmt(
							_("Overwrite `%s' in `%s'? "
								"There are %d more conflicts like this "
								"(y:Yes a:Yes to all n:No q:No to all)"),
							filename, directory.value(), --nfiles_exist),
						input_options);
			} else {
				c = v.confirm(strprintf::fmt(
							_("Overwrite `%s' in `%s'? "
								"(y:Yes n:No)"),
							filename, directory.value()),
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

void ItemListFormAction::apply_filter(const std::string& filtertext)
{
	if (filtertext.empty()) {
		return;
	}

	filterhistory.add_line(filtertext);
	if (!matcher.parse(filtertext)) {
		v.get_statusline().show_error(strprintf::fmt(
				_("Error: couldn't parse filter expression `%s': %s"),
				filtertext,
				matcher.get_parse_error()));
	} else {
		save_filterpos();
		filter_active = true;
		invalidate_list();
	}
}

} // namespace newsboat

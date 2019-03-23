#include "itemviewformaction.h"

#include <algorithm>
#include <cstring>
#include <sstream>

#include "config.h"
#include "exceptions.h"
#include "fmtstrformatter.h"
#include "itemrenderer.h"
#include "logger.h"
#include "strprintf.h"
#include "textformatter.h"
#include "utils.h"
#include "view.h"

namespace newsboat {

ItemViewFormAction::ItemViewFormAction(View* vv,
	std::shared_ptr<ItemListFormAction> il,
	std::string formstr,
	Cache* cc,
	ConfigContainer* cfg)
	: FormAction(vv, formstr, cfg)
	, show_source(false)
	, quit(false)
	, rxman(0)
	, num_lines(0)
	, itemlist(il)
	, in_search(false)
	, rsscache(cc)
{
	valid_cmds.push_back("save");
	std::sort(valid_cmds.begin(), valid_cmds.end());
}

ItemViewFormAction::~ItemViewFormAction() {}

void ItemViewFormAction::init()
{
	f->set("msg", "");
	do_redraw = true;
	quit = false;
	links.clear();
	num_lines = 0;
	if (!cfg->get_configvalue_as_bool("display-article-progress")) {
		f->set("percentwidth", "0");
	} else {
		const size_t min_field_width = 6;
		f->set("percentwidth",
			std::to_string(
				std::max({
					min_field_width,
					strlen(_("Top")),
					strlen(_("Bottom"))})));
		update_percent();
	}
	set_keymap_hints();
	item = feed->get_item_by_guid(guid);
}

void ItemViewFormAction::update_head(const std::shared_ptr<RssItem>& item)
{
	const std::string feedtitle = item_renderer::get_feedtitle(item);

	unsigned int unread_item_count = feed->unread_item_count();
	// we need to subtract because the current item isn't yet marked
	// as read
	if (item->unread()) {
		unread_item_count--;
	}
	set_head(item->title(),
		feedtitle,
		unread_item_count,
		feed->total_item_count());
};

void ItemViewFormAction::prepare()
{
	/*
	 * whenever necessary, the item view is regenerated. This is done
	 * by putting together the feed name, title, link, author, optional
	 * flags and podcast download URL (enclosures) and then render the
	 * HTML. The links extracted by the renderer are then appended, too.
	 */
	if (do_redraw) {
		{
			ScopeMeasure("itemview::prepare: rendering");
			// XXX HACK: render once so that we get a proper widget width
			f->run(-3);
		}

		update_head(item);

		const std::string widthstr = f->get("article:w");
		const unsigned int window_width = utils::to_u(widthstr, 0);

		unsigned int text_width =
			cfg->get_configvalue_as_int("text-width");
		if (text_width == 0 || text_width > window_width) {
			text_width = window_width;
			if (text_width - 5 > 0) {
				text_width -= 5;
			}
		}

		std::string formatted_text;
		if (show_source) {
			std::tie(formatted_text, num_lines) =
				item_renderer::source_to_stfl_list(
					item,
					text_width,
					window_width,
					rxman,
					"article");
		} else {
			std::tie(formatted_text, num_lines) =
				item_renderer::to_stfl_list(
					// cfg can't be nullptr because that's a long-lived object
					// created at the very start of the program.
					*cfg,
					item,
					text_width,
					window_width,
					rxman,
					"article",
					links);
		}

		f->modify("article", "replace_inner", formatted_text);
		f->set("articleoffset", "0");

		if (in_search) {
			rxman->remove_last_regex("article");
			in_search = false;
		}

		do_redraw = false;
	}
}

void ItemViewFormAction::process_operation(Operation op,
	bool automatic,
	std::vector<std::string>* args)
{
	bool hardquit = false;

	/*
	 * whenever we process an operation, we mark the item
	 * as read. Don't worry: when an item is already marked as
	 * read, and then marked as read again, no database update
	 * is done, since only _changes_ to the unread flag are
	 * recorded in the database.
	 */
	try {
		bool old_unread = item->unread();
		item->set_unread(false);
		if (old_unread) {
			v->get_ctrl()->mark_article_read(item->guid(), true);
		}
	} catch (const DbException& e) {
		v->show_error(strprintf::fmt(
			_("Error while marking article as read: %s"),
			e.what()));
	}

	switch (op) {
	case OP_TOGGLESOURCEVIEW:
		LOG(Level::INFO, "ItemViewFormAction::process_operation: toggling source view");
		show_source = !show_source;
		do_redraw = true;
		break;
	case OP_ENQUEUE: {
		if (item->enclosure_url().length() > 0 &&
			utils::is_http_url(item->enclosure_url())) {
			v->get_ctrl()->enqueue_url(item, feed);
			v->set_status(
				strprintf::fmt(_("Added %s to download queue."),
					item->enclosure_url()));
		} else {
			v->set_status(strprintf::fmt(
				_("Invalid URL: '%s'"), item->enclosure_url()));
		}
	} break;
	case OP_SAVE: {
		LOG(Level::INFO, "ItemViewFormAction::process_operation: saving article");
		std::string filename;
		if (automatic) {
			if (args->size() > 0)
				filename = (*args)[0];
		} else {
			filename = v->run_filebrowser(
				v->get_filename_suggestion(item->title()));
		}
		if (filename == "") {
			v->show_error(_("Aborted saving."));
		} else {
			try {
				v->get_ctrl()->write_item(item, filename);
				v->show_error(strprintf::fmt(
					_("Saved article to %s."), filename));
			} catch (...) {
				v->show_error(strprintf::fmt(
					_("Error: couldn't write article to "
					  "file %s"),
					filename));
			}
		}
	} break;
	case OP_OPENINBROWSER:
		LOG(Level::INFO, "ItemViewFormAction::process_operation: starting browser");
		v->set_status(_("Starting browser..."));
		v->open_in_browser(item->link());
		v->set_status("");
		break;
	case OP_BOOKMARK:
		if (automatic) {
			qna_responses.clear();
			qna_responses.push_back(item->link());
			qna_responses.push_back(item->title());
			qna_responses.push_back(
				args->size() > 0 ? (*args)[0] : "");
			qna_responses.push_back(feed->title());
		} else {
			this->start_bookmark_qna(
				item->title(), item->link(), "", feed->title());
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
	case OP_PIPE_TO: {
		std::vector<QnaPair> qna;
		if (automatic) {
			if (args->size() > 0) {
				qna_responses.clear();
				qna_responses.push_back((*args)[0]);
				finished_qna(OP_PIPE_TO);
			}
		} else {
			qna.push_back(
				QnaPair(_("Pipe article to command: "), ""));
			this->start_qna(qna, OP_PIPE_TO, &cmdlinehistory);
		}
	} break;
	case OP_EDITFLAGS:
		if (automatic) {
			qna_responses.clear();
			if (args->size() > 0) {
				qna_responses.push_back((*args)[0]);
				this->finished_qna(OP_INT_EDITFLAGS_END);
			}
		} else {
			std::vector<QnaPair> qna;
			qna.push_back(QnaPair(_("Flags: "), item->flags()));
			this->start_qna(qna, OP_INT_EDITFLAGS_END);
		}
		break;
	case OP_SHOWURLS: {
		std::string urlviewer =
			cfg->get_configvalue("external-url-viewer");
		LOG(Level::DEBUG, "ItemViewFormAction::process_operation: showing URLs");
		if (urlviewer == "") {
			if (links.size() > 0) {
				v->push_urlview(links, feed);
			} else {
				v->show_error(_("URL list empty."));
			}
		} else {
			qna_responses.clear();
			qna_responses.push_back(urlviewer);
			this->finished_qna(OP_PIPE_TO);
		}
	} break;
	case OP_DELETE:
		LOG(Level::INFO,
			"ItemViewFormAction::process_operation: deleting current article");
		item->set_deleted(true);
		rsscache->mark_item_deleted(guid, true);
	/* fall-through! */
	case OP_NEXTUNREAD:
		LOG(Level::INFO,
			"ItemViewFormAction::process_operation: jumping to next unread article");
		if (v->get_next_unread(itemlist.get(), this)) {
			do_redraw = true;
		} else {
			v->pop_current_formaction();
			v->show_error(_("No unread items."));
		}
		break;
	case OP_PREVUNREAD:
		LOG(Level::INFO,
			"ItemViewFormAction::process_operation: jumping to previous unread "
			"article");
		if (v->get_previous_unread(itemlist.get(), this)) {
			do_redraw = true;
		} else {
			v->pop_current_formaction();
			v->show_error(_("No unread items."));
		}
		break;
	case OP_NEXT:
		LOG(Level::INFO, "ItemViewFormAction::process_operation: jumping to next article");
		if (v->get_next(itemlist.get(), this)) {
			do_redraw = true;
		} else {
			v->pop_current_formaction();
			v->show_error(_("Already on last item."));
		}
		break;
	case OP_PREV:
		LOG(Level::INFO,
			"ItemViewFormAction::process_operation: jumping to previous article");
		if (v->get_previous(itemlist.get(), this)) {
			do_redraw = true;
		} else {
			v->pop_current_formaction();
			v->show_error(_("Already on first item."));
		}
		break;
	case OP_RANDOMUNREAD:
		LOG(Level::INFO,
			"ItemViewFormAction::process_operation: jumping to random unread article");
		if (v->get_random_unread(itemlist.get(), this)) {
			do_redraw = true;
		} else {
			v->pop_current_formaction();
			v->show_error(_("No unread items."));
		}
		break;
	case OP_TOGGLEITEMREAD:
		LOG(Level::INFO,
			"ItemViewFormAction::process_operation: setting unread and quitting");
		v->set_status(_("Toggling read flag for article..."));
		try {
			item->set_unread(true);
			v->get_ctrl()->mark_article_read(item->guid(), false);
		} catch (const DbException& e) {
			v->show_error(strprintf::fmt(
				_("Error while marking article as unread: %s"),
				e.what()));
		}
		v->set_status("");
		quit = true;
		break;
	case OP_QUIT:
		LOG(Level::INFO, "ItemViewFormAction::process_operation: quitting");
		quit = true;
		break;
	case OP_HARDQUIT:
		LOG(Level::INFO, "ItemViewFormAction::process_operation: hard quitting");
		hardquit = true;
		break;
	case OP_HELP:
		v->push_help();
		break;
	case OP_1:
	case OP_2:
	case OP_3:
	case OP_4:
	case OP_5:
	case OP_6:
	case OP_7:
	case OP_8:
	case OP_9:
	case OP_0: {
		unsigned int idx = op - OP_1;
		LOG(Level::DEBUG,
			"ItemViewFormAction::process_operation: OP_1 = %d op = %d idx = %u",
			OP_1,
			op,
			idx);
		if (idx < links.size()) {
			v->set_status(_("Starting browser..."));
			v->open_in_browser(links[idx].first);
			v->set_status("");
		}
	} break;
	case OP_GOTO_URL: {
		std::vector<QnaPair> qna;
		if (automatic) {
			if (args->size() > 0) {
				qna_responses.clear();
				qna_responses.push_back((*args)[0]);
				finished_qna(OP_INT_GOTO_URL);
			}
		} else {
			qna.push_back(QnaPair(_("Goto URL #"), ""));
			this->start_qna(qna, OP_INT_GOTO_URL);
		}
	} break;
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

KeyMapHintEntry* ItemViewFormAction::get_keymap_hint()
{
	static KeyMapHintEntry hints[] = {{OP_QUIT, _("Quit")},
		{OP_SAVE, _("Save")},
		{OP_NEXTUNREAD, _("Next Unread")},
		{OP_OPENINBROWSER, _("Open in Browser")},
		{OP_ENQUEUE, _("Enqueue")},
		{OP_HELP, _("Help")},
		{OP_NIL, nullptr}};
	return hints;
}

void ItemViewFormAction::set_head(const std::string& s,
	const std::string& feedtitle,
	unsigned int unread,
	unsigned int total)
{
	FmtStrFormatter fmt;
	fmt.register_fmt('N', PROGRAM_NAME);
	fmt.register_fmt('V', PROGRAM_VERSION);

	auto itemtitle = s;
	utils::remove_soft_hyphens(itemtitle);
	fmt.register_fmt('T', itemtitle);

	auto clear_feedtitle = feedtitle;
	utils::remove_soft_hyphens(clear_feedtitle);
	fmt.register_fmt('F', clear_feedtitle);

	fmt.register_fmt('u', std::to_string(unread));
	fmt.register_fmt('t', std::to_string(total));

	std::string listwidth = f->get("article:w");
	unsigned int width = utils::to_u(listwidth);

	f->set("head",
		fmt.do_format(
			cfg->get_configvalue("itemview-title-format"), width));
}

void ItemViewFormAction::handle_cmdline(const std::string& cmd)
{
	std::vector<std::string> tokens = utils::tokenize_quoted(cmd);
	if (!tokens.empty()) {
		if (tokens[0] == "save" && tokens.size() >= 2) {
			std::string filename = utils::resolve_tilde(tokens[1]);

			if (filename == "") {
				v->show_error(_("Aborted saving."));
			} else {
				try {
					v->get_ctrl()->write_item(
						item, filename);
					v->show_error(strprintf::fmt(
						_("Saved article to %s"),
						filename));
				} catch (...) {
					v->show_error(strprintf::fmt(
						_("Error: couldn't save "
						  "article to %s"),
						filename));
				}
			}

		} else {
			FormAction::handle_cmdline(cmd);
		}
	}
}

void ItemViewFormAction::finished_qna(Operation op)
{
	FormAction::finished_qna(op); // important!

	switch (op) {
	case OP_INT_EDITFLAGS_END:
		item->set_flags(qna_responses[0]);
		v->get_ctrl()->update_flags(item);
		v->set_status(_("Flags updated."));
		do_redraw = true;
		break;
	case OP_INT_START_SEARCH:
		do_search();
		break;
	case OP_PIPE_TO: {
		std::string cmd = qna_responses[0];
		std::ostringstream ostr;
		v->get_ctrl()->write_item(feed->get_item_by_guid(guid), ostr);
		v->push_empty_formaction();
		Stfl::reset();
		FILE* f = popen(cmd.c_str(), "w");
		if (f) {
			std::string data = ostr.str();
			fwrite(data.c_str(), data.length(), 1, f);
			pclose(f);
		}
		v->pop_current_formaction();
	} break;
	case OP_INT_GOTO_URL: {
		unsigned int idx = 0;
		sscanf(qna_responses[0].c_str(), "%u", &idx);
		if (idx && idx - 1 < links.size()) {
			v->set_status(_("Starting browser..."));
			v->open_in_browser(links[idx - 1].first);
			v->set_status("");
		}
	} break;
	default:
		break;
	}
}

void ItemViewFormAction::set_regexmanager(RegexManager* r)
{
	rxman = r;
	std::vector<std::string>& attrs = r->get_attrs("article");
	unsigned int i = 0;
	std::string attrstr;
	for (const auto& attribute : attrs) {
		attrstr.append(
			strprintf::fmt("@style_%u_normal:%s ", i, attribute));
		i++;
	}
	attrstr.append(
		"@style_b_normal[color_bold]:attr=bold "
		"@style_u_normal[color_underline]:attr=underline ");
	std::string textview = strprintf::fmt(
		"{textview[article] style_normal[article]: "
		"style_end[styleend]:fg=blue,attr=bold %s .expand:vh "
		"offset[articleoffset]:0 richtext:1}",
		attrstr);
	f->modify("article", "replace", textview);
}

void ItemViewFormAction::update_percent()
{
	if (cfg->get_configvalue_as_bool("display-article-progress")) {
		unsigned int percent = 0;
		unsigned int offset = utils::to_u(f->get("articleoffset"), 0);

		if (num_lines > 0)
			percent = (100 * (offset + 1)) / num_lines;
		else
			percent = 0;

		LOG(Level::DEBUG,
			"ItemViewFormAction::update_percent: offset = %u "
			"num_lines = %u percent = %u",
			offset,
			num_lines,
			percent);

		if (offset == 0 || percent == 0) {
			f->set("percent", _("Top"));
		} else if (offset == (num_lines - 1)) {
			f->set("percent", _("Bottom"));
		} else {
			f->set("percent", strprintf::fmt("%3u %% ", percent));
		}
	}
}

std::string ItemViewFormAction::title()
{
	auto title = item->title();
	utils::remove_soft_hyphens(title);
	return strprintf::fmt(_("Article - %s"), title);
}

void ItemViewFormAction::set_highlightphrase(const std::string& text)
{
	highlight_text(text);
}

void ItemViewFormAction::do_search()
{
	std::string searchphrase = qna_responses[0];
	if (searchphrase.length() == 0)
		return;

	searchhistory.add_line(searchphrase);

	LOG(Level::DEBUG,
		"ItemViewFormAction::do_search: searchphrase = %s",
		searchphrase);

	highlight_text(searchphrase);
}

void ItemViewFormAction::highlight_text(const std::string& searchphrase)
{
	std::vector<std::string> params;
	params.push_back("article");
	params.push_back(searchphrase);

	std::vector<std::string> colors = utils::tokenize(
		cfg->get_configvalue("search-highlight-colors"), " ");
	std::copy(colors.begin(), colors.end(), std::back_inserter(params));

	try {
		rxman->handle_action("highlight", params);

		LOG(Level::DEBUG,
			"ItemViewFormAction::highlight_text: configuration "
			"manipulation was successful");

		set_regexmanager(rxman);

		in_search = true;
		do_redraw = true;
	} catch (const ConfigHandlerException& e) {
		LOG(Level::ERROR,
			"ItemViewFormAction::highlight_text: handle_action "
			"failed, error = %s",
			e.what());
		v->show_error(_("Error: invalid regular expression!"));
	}
}

} // namespace newsboat

#include <itemview_formaction.h>
#include <view.h>
#include <config.h>
#include <logger.h>
#include <exceptions.h>
#include <utils.h>

#include <sstream>

namespace newsbeuter {

itemview_formaction::itemview_formaction(view * vv, std::string formstr)
	: formaction(vv,formstr), feed(0), show_source(false), quit(false) { 
}

itemview_formaction::~itemview_formaction() { }


void itemview_formaction::init() {
	f->set("msg","");
	do_redraw = true;
	quit = false;
	links.erase(links.begin(), links.end());
	set_keymap_hints();
}

void itemview_formaction::prepare() {
	static bool render_hack;

	/*
	 * whenever necessary, the item view is regenerated. This is done
	 * by putting together the feed name, title, link, author, optional
	 * flags and podcast download URL (enclosures) and then render the
	 * HTML. The links extracted by the renderer are then appended, too.
	 */
	if (do_redraw) {
		rss_item& item = feed->get_item_by_guid(guid);
		std::string code = "{list";

		rss_feed * feedptr = item.get_feedptr();

		code.append("{listitem text:");
		std::ostringstream feedtitle;
		feedtitle << _("Feed: ");
		if (feedptr->title().length() > 0) {
			feedtitle << feedptr->title();
		} else if (feedptr->link().length() > 0) {
			feedtitle << feedptr->link();
		} else if (feedptr->rssurl().length() > 0) {
			feedtitle << feedptr->rssurl();
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

		if (item.flags().length() > 0) {
			code.append("{listitem text:");
			std::ostringstream flags;
			flags << _("Flags: ");
			flags << item.flags();
			code.append(stfl::quote(flags.str()));
			code.append("}");
		}

		if (item.enclosure_url().length() > 0) {
			code.append("{listitem text:");
			std::ostringstream enc_url;
			enc_url << _("Podcast Download URL: ");
			enc_url << item.enclosure_url() << " (" << _("type: ") << item.enclosure_type() << ")";
			code.append(stfl::quote(enc_url.str()));
			code.append("}");
		}

		code.append("{listitem text:\"\"}");
		
		set_head(item.title());

		if (!render_hack) {
			f->run(-1); // XXX HACK: render once so that we get a proper widget width
			render_hack = true;
		}

		std::vector<std::string> lines;
		std::string widthstr = f->get("article:w");
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
			lines = render_html(item.description(), links, item.feedurl(), render_width);
		}

		for (std::vector<std::string>::iterator it = lines.begin(); it != lines.end(); ++it) {
			std::string line("{listitem text:");
			line.append(stfl::quote(*it));
			line.append(1,'}');
			code.append(line);
		}

		code.append("}");

		f->modify("article","replace_inner",code);
		f->set("articleoffset","0");

		do_redraw = false;
	}

}

void itemview_formaction::process_operation(operation op) {
	rss_item& item = feed->get_item_by_guid(guid);

	/*
	 * whenever we process an operation, we mark the item
	 * as read. Don't worry: when an item is already marked as
	 * read, and then marked as read again, no database update
	 * is done, since only _changes_ to the unread flag are
	 * recorded in the database.
	 */
	try {
		item.set_unread(false);
	} catch (const dbexception& e) {
		char buf[1024];
		snprintf(buf, sizeof(buf), _("Error while marking article as read: %s"), e.what());
		v->show_error(buf);
	}

	switch (op) {
		case OP_OPEN:
			// nothing
			break;
		case OP_TOGGLESOURCEVIEW:
			GetLogger().log(LOG_INFO, "view::run_itemview: toggling source view");
			show_source = !show_source;
			do_redraw = true;
			break;
		case OP_ENQUEUE: {
				char buf[1024];
				if (item.enclosure_url().length() > 0) {
					snprintf(buf, sizeof(buf), _("Added %s to download queue."), item.enclosure_url().c_str());
					v->get_ctrl()->enqueue_url(item.enclosure_url());
					v->set_status(buf);
				}
			}
			break;
		case OP_SAVE:
			{
				char buf[1024];
				GetLogger().log(LOG_INFO, "view::run_itemview: saving article");
				std::string filename = v->run_filebrowser(FBT_SAVE,v->get_filename_suggestion(item.title()));
				if (filename == "") {
					v->show_error(_("Aborted saving."));
				} else {
					try {
						v->write_item(item, filename);
						snprintf(buf, sizeof(buf), _("Saved article to %s."), filename.c_str());
						v->show_error(buf);
					} catch (...) {
						snprintf(buf, sizeof(buf), _("Error: couldn't write article to file %s"), filename.c_str());
						v->show_error(buf);
					}
				}
			}
			break;
		case OP_OPENINBROWSER:
			GetLogger().log(LOG_INFO, "view::run_itemview: starting browser");
			v->set_status(_("Starting browser..."));
			v->open_in_browser(item.link());
			v->set_status("");
			break;
		case OP_BOOKMARK:
			this->start_bookmark_qna(item.title(), item.link(), "");
			break;
		case OP_EDITFLAGS: {
				std::vector<qna_pair> qna;
				qna.push_back(qna_pair(_("Flags: "), item.flags()));
				this->start_qna(qna, OP_INT_EDITFLAGS_END);
			}
			break;
		case OP_SHOWURLS:
			GetLogger().log(LOG_DEBUG, "view::run_itemview: showing URLs");
			if (links.size() > 0) {
				v->push_urlview(links);
			} else {
				v->show_error(_("URL list empty."));
			}
			break;
		case OP_NEXTUNREAD:
			GetLogger().log(LOG_INFO, "view::run_itemview: jumping to next unread article");
			if (v->get_next_unread()) {
				do_redraw = true;
			} else {
				v->pop_current_formaction();
				v->show_error(_("No unread items."));
			}
			break;
		case OP_PREVUNREAD:
			GetLogger().log(LOG_INFO, "view::run_itemview: jumping to previous unread article");
			if (v->get_previous_unread()) {
				do_redraw = true;
			} else {
				v->pop_current_formaction();
				v->show_error(_("No unread items."));
			}
			break;
		case OP_QUIT:
			GetLogger().log(LOG_INFO, "view::run_itemview: quitting");
			quit = true;
			break;
		case OP_HELP:
			v->push_help();
			break;
		default:
			break;
	}

	if (quit) {
		v->pop_current_formaction();
	}

}

keymap_hint_entry * itemview_formaction::get_keymap_hint() {
	static keymap_hint_entry hints[] = {
		{ OP_QUIT, _("Quit") },
		{ OP_OPEN, _("Open") },
		{ OP_SAVE, _("Save") },
		{ OP_NEXTUNREAD, _("Next Unread") },
		{ OP_OPENINBROWSER, _("Open in Browser") },
		{ OP_ENQUEUE, _("Enqueue") },
		{ OP_HELP, _("Help") },
		{ OP_NIL, NULL }
	};
	return hints;
}

void itemview_formaction::set_head(const std::string& s) {
	char buf[1024];
	snprintf(buf, sizeof(buf), _("%s %s - Article '%s'"), PROGRAM_NAME, PROGRAM_VERSION, s.c_str());
	f->set("head",buf);
}

void itemview_formaction::render_source(std::vector<std::string>& lines, std::string desc, unsigned int width) {
	/*
	 * this function is called instead of htmlrenderer::render() when the
	 * user requests to have the source displayed instead of seeing the
	 * rendered HTML.
	 */
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

void itemview_formaction::handle_cmdline(const std::string& cmd) {
	std::vector<std::string> tokens = utils::tokenize_quoted(cmd);
	if (tokens.size() > 0) {
		if (tokens[0] == "save" && tokens.size() >= 2) {
			std::string filename = utils::resolve_tilde(tokens[1]);
			rss_item& item = feed->get_item_by_guid(guid);
			char buf[1024];

			if (filename == "") {
				v->show_error(_("Aborted saving."));
			} else {
				try {
					v->write_item(item, filename);
					snprintf(buf, sizeof(buf), _("Saved article to %s"), filename.c_str());
					v->show_error(buf);
				} catch (...) {
					snprintf(buf, sizeof(buf), _("Error: couldn't save article to %s"), filename.c_str());
					v->show_error(buf);
				}
			}

		} else {
			formaction::handle_cmdline(cmd);
		}
	}
}

void itemview_formaction::finished_qna(operation op) {
	formaction::finished_qna(op); // important!

	rss_item& item = feed->get_item_by_guid(guid);

	switch (op) {
		case OP_INT_EDITFLAGS_END:
			item.set_flags(qna_responses[0]);
			item.update_flags();
			v->set_status(_("Flags updated."));
			do_redraw = true;
			break;
		default:
			break;
	}
}

std::vector<std::string> itemview_formaction::render_html(const std::string& source, std::vector<linkpair>& links, const std::string& feedurl, unsigned int render_width) {
	std::vector<std::string> lines;
	std::string renderer = v->get_cfg()->get_configvalue("html-renderer");
	if (renderer == "internal") {
		htmlrenderer rnd(render_width);
		rnd.render(source, lines, links, feedurl);
	} else {
		std::string output = utils::run_filter(renderer, source);
		std::istringstream is(output);
		std::string line;
		getline(is, line);
		while (!is.eof()) {
			lines.push_back(line);
			getline(is, line);
		}
	}
	return lines;
}

}

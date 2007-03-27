#include <itemview_formaction.h>
#include <view.h>
#include <config.h>
#include <logger.h>

#include <sstream>

namespace newsbeuter {

itemview_formaction::itemview_formaction(view * vv, std::string formstr)
	: formaction(vv,formstr), feed(0), show_source(false), quit(false) { }

itemview_formaction::~itemview_formaction() { }


void itemview_formaction::init() {
	f->set("msg","");
	do_redraw = true;
	quit = false;
	links.erase(links.begin(), links.end());
}

void itemview_formaction::prepare() {
	static bool render_hack;

	if (do_redraw) {
		rss_item& item = feed->get_item_by_guid(guid);
		std::string code = "{list";

		code.append("{listitem text:");
		std::ostringstream feedtitle;
		feedtitle << _("Feed: ");
		if (feed->title().length() > 0) {
			feedtitle << feed->title();
		} else if (feed->link().length() > 0) {
			feedtitle << feed->link();
		} else if (feed->rssurl().length() > 0) {
			feedtitle << feed->rssurl();
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

		if (item.enclosure_url().length() > 0) {
			code.append("{listitem text:");
			std::ostringstream enc_url;
			enc_url << _("Podcast Download URL: ");
			enc_url << item.enclosure_url() << " (" << _("type: ") << item.enclosure_type() << ")";
			code.append(stfl::quote(enc_url.str()));
			code.append("}");
		}

		code.append("{listitem text:\"\"}");
		
		// set_itemview_head(item.title());

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
			v->render_source(lines, item.description(), render_width);
		} else {
			htmlrenderer rnd(render_width);
			rnd.render(item.description(), lines, links, item.feedurl());
		}

		for (std::vector<std::string>::iterator it = lines.begin(); it != lines.end(); ++it) {
			std::string line = std::string("{listitem text:") + stfl::quote(*it) + std::string("}");
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
				snprintf(buf, sizeof(buf), _("Added %s to download queue."), item.enclosure_url().c_str());
				v->get_ctrl()->enqueue_url(item.enclosure_url());
				v->set_status(buf);
			}
			break;
		case OP_SAVE:
			{
				char buf[1024];
				GetLogger().log(LOG_INFO, "view::run_itemview: saving article");
				std::string filename; // = filebrowser(FBT_SAVE,get_filename_suggestion(item.title()));
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
		case OP_SHOWURLS:
			GetLogger().log(LOG_DEBUG, "view::run_itemview: showing URLs");
			// run_urlview(links);
			break;
		case OP_NEXTUNREAD:
			GetLogger().log(LOG_INFO, "view::run_itemview: jumping to next unread article");
			// retval = true; // fall-through is OK
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


}

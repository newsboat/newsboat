#include "itemrenderer.h"

#include <sstream>

#include "configcontainer.h"
#include "htmlrenderer.h"
#include "rss.h"
#include "textformatter.h"

namespace newsboat {

std::string item_renderer::get_feedtitle(std::shared_ptr<RssItem> item) {
	const std::shared_ptr<RssFeed> feedptr = item->get_feedptr();

	std::string feedtitle;
	if (feedptr) {
		if (!feedptr->title().empty()) {
			feedtitle = feedptr->title();
			utils::remove_soft_hyphens(feedtitle);
		} else if (!feedptr->link().empty()) {
			feedtitle = feedptr->link();
		} else if (!feedptr->rssurl().empty()) {
			feedtitle = feedptr->rssurl();
		}
	}

	return feedtitle;
}

void prepare_header(
	std::shared_ptr<RssItem> item,
	std::vector<std::pair<LineType, std::string>>& lines,
	std::vector<LinkPair>& /*links*/)
{
	const auto add_line =
		[&lines]
		(const std::string& value,
		 const std::string& name,
		 LineType lineType = LineType::wrappable)
		{
			if (!value.empty()) {
				const auto line = strprintf::fmt("%s%s", name, value);
				lines.push_back(std::make_pair(lineType, line));
			}
		};

	const std::string feedtitle = item_renderer::get_feedtitle(item);
	add_line(feedtitle, _("Feed: "));
	add_line(item->title(), _("Title: "));
	add_line(item->author(), _("Author: "));
	add_line(item->pubDate(), _("Date: "));
	add_line(item->link(), _("Link: "), LineType::softwrappable);
	add_line(item->flags(), _("Flags: "));

	if (!item->enclosure_url().empty()) {
		auto dlurl = strprintf::fmt(
			"%s%s",
			_("Podcast Download URL: "),
			utils::censor_url(item->enclosure_url()));
		if (!item->enclosure_type().empty()) {
			dlurl.append(
					strprintf::fmt(" (%s%s)",
						_("type: "),
						item->enclosure_type()));
		}
		lines.push_back(std::make_pair(LineType::softwrappable, dlurl));
	}

	lines.push_back(std::make_pair(LineType::wrappable, std::string("")));
}

std::string get_item_base_link(const std::shared_ptr<RssItem>& item)
{
	if (!item->get_base().empty()) {
		return item->get_base();
	} else {
		return item->feedurl();
	}
}

void render_html(
	ConfigContainer& cfg,
	const std::string& source,
	std::vector<std::pair<LineType, std::string>>& lines,
	std::vector<LinkPair>& thelinks,
	const std::string& url,
	bool raw)
{
	const std::string renderer = cfg.get_configvalue("html-renderer");
	if (renderer == "internal") {
		HtmlRenderer rnd(raw);
		rnd.render(source, lines, thelinks, url);
	} else {
		char* argv[4];
		argv[0] = const_cast<char*>("/bin/sh");
		argv[1] = const_cast<char*>("-c");
		argv[2] = const_cast<char*>(renderer.c_str());
		argv[3] = nullptr;
		LOG(Level::DEBUG,
			"item_renderer::render_html: source = %s",
			source);
		LOG(Level::DEBUG,
			"item_renderer::render_html: html-renderer = %s",
			argv[2]);

		const std::string output = utils::run_program(argv, source);
		std::istringstream is(output);
		std::string line;
		while (!is.eof()) {
			getline(is, line);
			if (!raw) {
				line = utils::quote_for_stfl(line);
			}
			lines.push_back(std::make_pair(LineType::softwrappable, line));
		}
	}
}

std::string item_renderer::to_plain_text(
		ConfigContainer& cfg,
		std::shared_ptr<RssItem> item)
{
	std::vector<std::pair<LineType, std::string>> lines;
	std::vector<LinkPair> links;

	prepare_header(item, lines, links);
	const auto base = get_item_base_link(item);
	render_html(cfg, item->description(), lines, links, base, true);

	TextFormatter txtfmt;
	txtfmt.add_lines(lines);

	unsigned int width = cfg.get_configvalue_as_int("text-width");
	if (width == 0) {
		width = 80;
	}

	return txtfmt.format_text_plain(width);
}

std::pair<std::string, size_t> item_renderer::to_stfl_list(
		ConfigContainer& cfg,
		std::shared_ptr<RssItem> item,
		unsigned int text_width,
		unsigned int window_width,
		RegexManager* rxman,
		const std::string& location,
		std::vector<LinkPair>& links)
{
	std::vector<std::pair<LineType, std::string>> lines;

	prepare_header(item, lines, links);
	const std::string baseurl = get_item_base_link(item);
	const auto body = item->description();
	render_html(cfg, body, lines, links, baseurl, false);

	TextFormatter txtfmt;
	txtfmt.add_lines(lines);

	return txtfmt.format_text_to_list(rxman, location, text_width, window_width);
}

void render_source(
	std::vector<std::pair<LineType, std::string>>& lines,
	std::string source)
{
	/*
	 * This function is called instead of HtmlRenderer::render() when the
	 * user requests to have the source displayed instead of seeing the
	 * rendered HTML.
	 */
	std::string line;
	do {
		std::string::size_type pos = source.find_first_of("\r\n");
		line = source.substr(0, pos);
		if (pos == std::string::npos) {
			source.erase();
		} else {
			source.erase(0, pos + 1);
		}
		lines.push_back(std::make_pair(LineType::softwrappable, line));
	} while (source.length() > 0);
}

std::pair<std::string, size_t> item_renderer::source_to_stfl_list(
		std::shared_ptr<RssItem> item,
		unsigned int text_width,
		unsigned int window_width,
		RegexManager* rxman,
		const std::string& location)
{
	std::vector<std::pair<LineType, std::string>> lines;
	std::vector<LinkPair> links;

	prepare_header(item, lines, links);
	render_source(lines, utils::quote_for_stfl(item->description()));

	TextFormatter txtfmt;
	txtfmt.add_lines(lines);

	return txtfmt.format_text_to_list(rxman, location, text_width, window_width);
}

}

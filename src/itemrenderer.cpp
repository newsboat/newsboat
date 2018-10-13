#include "itemrenderer.h"

#include <sstream>

#include "configcontainer.h"
#include "htmlrenderer.h"
#include "rss.h"
#include "textformatter.h"

namespace newsboat {

std::string item_renderer::get_feedtitle(std::shared_ptr<RssItem> item) {
	std::shared_ptr<RssFeed> feedptr = item->get_feedptr();

	std::string feedtitle;
	if (feedptr) {
		if (!feedptr->title().empty()) {
			feedtitle = feedptr->title();
			Utils::remove_soft_hyphens(feedtitle);
		} else if (!feedptr->link().empty()) {
			feedtitle = feedptr->link();
		} else if (!feedptr->rssurl().empty()) {
			feedtitle = feedptr->rssurl();
		}
	}

	return feedtitle;
}

void item_renderer::prepare_header(
	std::shared_ptr<RssItem> item,
	std::vector<std::pair<LineType, std::string>>& lines,
	std::vector<LinkPair>& /*links*/)
{
	const auto feedtitle = item_renderer::get_feedtitle(item);
	if (!feedtitle.empty()) {
		const auto title = StrPrintf::fmt("%s%s", _("Feed: "), feedtitle);
		lines.push_back(std::make_pair(LineType::wrappable, title));
	}

	if (!item->title().empty()) {
		const auto title = StrPrintf::fmt("%s%s", _("Title: "), item->title());
		lines.push_back(std::make_pair(LineType::wrappable, title));
	}

	if (!item->author().empty()) {
		const auto author = StrPrintf::fmt("%s%s", _("Author: "), item->author());
		lines.push_back(std::make_pair(LineType::wrappable, author));
	}

	const auto date = StrPrintf::fmt("%s%s", _("Date: "), item->pubDate());
	lines.push_back(std::make_pair(LineType::wrappable, date));

	if (!item->link().empty()) {
		const auto link = StrPrintf::fmt("%s%s", _("Link: "), item->link());
		lines.push_back(std::make_pair(LineType::softwrappable, link));
	}

	if (!item->flags().empty()) {
		const auto flags = StrPrintf::fmt("%s%s", _("Flags: "), item->flags());
		lines.push_back(std::make_pair(LineType::wrappable, flags));
	}

	if (!item->enclosure_url().empty()) {
		auto dlurl = StrPrintf::fmt(
			"%s%s",
			_("Podcast Download URL: "),
			Utils::censor_url(item->enclosure_url()));
		if (!item->enclosure_type().empty()) {
			dlurl.append(StrPrintf::fmt("%s%s", _("type: "), item->enclosure_type()));
		}
		lines.push_back(std::make_pair(LineType::softwrappable, dlurl));
	}

	lines.push_back(std::make_pair(LineType::wrappable, std::string("")));
}

std::string item_renderer::get_item_base_link(const std::shared_ptr<RssItem>& item)
{
	std::string baseurl;
	if (!item->get_base().empty()) {
		baseurl = item->get_base();
	} else {
		baseurl = item->feedurl();
	}

	return baseurl;
}

void item_renderer::render_html(
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

		const std::string output = Utils::run_program(argv, source);
		std::istringstream is(output);
		std::string line;
		while (!is.eof()) {
			getline(is, line);
			if (!raw) {
				line = Utils::quote_for_stfl(line);
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
		const std::string& location)
{
	std::vector<std::pair<LineType, std::string>> lines;
	std::vector<LinkPair> links;

	prepare_header(item, lines, links);
	const std::string baseurl = item_renderer::get_item_base_link(item);
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
	render_source(lines, Utils::quote_for_stfl(item->description()));

	TextFormatter txtfmt;
	txtfmt.add_lines(lines);

	return txtfmt.format_text_to_list(rxman, location, text_width, window_width);
}

}

#include "itemrenderer.h"

#include <set>
#include <sstream>

#include "configcontainer.h"
#include "htmlrenderer.h"
#include "logger.h"
#include "rssfeed.h"
#include "strprintf.h"
#include "textformatter.h"
#include "utils.h"

namespace Newsboat {

bool should_render_as_html(const std::string mime_type)
{
	static const std::set<std::string> html_mime_types = {
		"", // Render as HTML when no mime-type is provided
		"html",
		"xhtml",
		"text/html",
		"application/xhtml+xml"
	};
	return (html_mime_types.count(mime_type) >= 1);
}

std::string item_renderer::get_feedtitle(RssItem& item)
{
	const std::shared_ptr<RssFeed> feedptr = item.get_feedptr();

	if (!feedptr) {
		return {};
	}

	std::string feedtitle;
	if (!feedptr->title().empty()) {
		feedtitle = feedptr->title();
		utils::remove_soft_hyphens(feedtitle);
	} else if (!feedptr->link().empty()) {
		feedtitle = feedptr->link();
	} else if (!feedptr->rssurl().empty()) {
		feedtitle = feedptr->rssurl();
	}

	return feedtitle;
}

void prepare_header(
	RssItem& item,
	std::vector<std::pair<LineType, std::string>>& lines,
	Links& /*links*/, bool raw = false)
{
	const auto add_line =
		[&lines]
	(const std::string& value, const std::string& name, LineType lineType) {
		if (!value.empty()) {
			const auto line = strprintf::fmt("%s%s", name, value);
			lines.push_back(std::make_pair(lineType, line));
		}
	};

	const auto stfl_quote_if_needed =
		[raw]
	(const std::string& str) {
		if (raw) {
			return str;
		} else {
			return utils::quote_for_stfl(str);
		};
	};

	const std::string feedtitle = item_renderer::get_feedtitle(item);
	add_line(stfl_quote_if_needed(feedtitle), _("Feed: "), LineType::wrappable);
	add_line(stfl_quote_if_needed(utils::utf8_to_locale(item.title())),
		_("Title: "), LineType::wrappable);
	add_line(stfl_quote_if_needed(utils::utf8_to_locale(item.author())),
		_("Author: "), LineType::wrappable);
	add_line(item.pubDate(), _("Date: "), LineType::wrappable);
	add_line(item.link(), _("Link: "), LineType::softwrappable);
	add_line(item.flags(), _("Flags: "), LineType::wrappable);

	const bool could_be_podcast = item.enclosure_type().empty()
		|| utils::is_valid_podcast_type(item.enclosure_type());

	if (!item.enclosure_url().empty() && could_be_podcast) {
		auto dlurl = strprintf::fmt(
				"%s%s",
				_("Podcast Download URL: "),
				utils::censor_url(item.enclosure_url()));
		if (!item.enclosure_type().empty()) {
			dlurl.append(
				strprintf::fmt(" (%s%s)",
					_("type: "),
					item.enclosure_type()));
		}
		lines.push_back(std::make_pair(LineType::softwrappable, dlurl));
	}

	lines.push_back(std::make_pair(LineType::wrappable, std::string("")));
}

std::string get_item_base_link(const RssItem& item)
{
	if (!item.get_base().empty()) {
		return item.get_base();
	} else if (!item.link().empty()) {
		return item.link();
	} else {
		return item.feedurl();
	}
}

void prepare_enclosure(RssItem& item,
	std::vector<std::pair<LineType, std::string>>& lines,
	Links& links,
	bool raw)
{
	const auto enclosure_url = item.enclosure_url();
	const auto enclosure_type = item.enclosure_type();
	const auto enclosure_description = item.enclosure_description();

	if (!enclosure_url.empty()
		&& !enclosure_type.empty()
		&& !utils::is_valid_podcast_type(enclosure_type)) {
		const bool is_image = (enclosure_type.find("image/") == 0);
		const auto link_type = is_image ? LinkType::IMG : LinkType::HREF;
		const std::string fallback_description = is_image ? _("Image") : _("Link");
		const std::string description = (enclosure_description.empty() ? fallback_description :
				enclosure_description);
		const auto link = utils::censor_url(
				utils::absolute_url(get_item_base_link(item), enclosure_url));
		const auto link_number = links.add_link(link, link_type);

		std::string text = "";
		if (raw) {
			text.append(strprintf::fmt("%s", description));
		} else {
			text.append(strprintf::fmt("<u>%s</>", utils::quote_for_stfl(description)));
		}
		text.append(strprintf::fmt("[%d]", link_number));

		lines.push_back({LineType::wrappable, text});
		lines.push_back({LineType::wrappable, ""});
	}
}

void render_html(
	ConfigContainer& cfg,
	const std::string& source,
	std::vector<std::pair<LineType, std::string>>& lines,
	Links& thelinks,
	const std::string& url,
	bool raw)
{
	const std::string renderer = cfg.get_configvalue("html-renderer");
	if (renderer == "internal") {
		HtmlRenderer rnd(raw);
		rnd.render(source, lines, thelinks, url);
	} else {
		const char* argv[4];
		argv[0] = "/bin/sh";
		argv[1] = "-c";
		argv[2] = renderer.c_str();
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

void item_renderer::render_plaintext(
	const std::string& source,
	std::vector<std::pair<LineType, std::string>>& lines, OutputFormat format)
{
	std::string normalized = utils::replace_all(source, "\r\n", "\n");
	normalized = utils::replace_all(normalized, "\r", "\n");

	std::string::size_type pos = 0;
	while (pos < normalized.size()) {
		const auto end_of_line = normalized.find_first_of("\n", pos);
		const std::string line = normalized.substr(pos, end_of_line - pos);

		switch (format) {
		case OutputFormat::PlainText:
			lines.push_back(std::make_pair(LineType::wrappable, line));
			break;
		case OutputFormat::StflRichText:
			const std::string stfl_quoted_line = utils::quote_for_stfl(line);
			lines.push_back(std::make_pair(LineType::wrappable, stfl_quoted_line));
			break;
		}

		if (end_of_line == std::string::npos) {
			break;
		}
		pos = end_of_line + 1;
	}
}

void render_links_summary(std::vector<std::pair<LineType, std::string>>& lines,
	const Links& links)
{
	if (links.size() > 0) {
		lines.push_back(std::make_pair(LineType::wrappable, ""));
		lines.push_back(std::make_pair(LineType::wrappable, _("Links: ")));
		for (unsigned int i = 0; i < links.size(); ++i) {
			auto link_text = strprintf::fmt("[%u]: %s (%s)",
					i + 1,
					links[i].url,
					Links::type2str(links[i].type));
			lines.push_back(std::make_pair(LineType::softwrappable, link_text));
		}
	}
}

std::string item_renderer::to_plain_text(
	ConfigContainer& cfg,
	RssItem& item)
{
	std::vector<std::pair<LineType, std::string>> lines;
	Links links;
	const auto item_description = item.description();

	prepare_header(item, lines, links, true);
	prepare_enclosure(item, lines, links, true);

	const auto base = get_item_base_link(item);
	const auto body = utils::utf8_to_locale(item_description.text);

	if (should_render_as_html(item_description.mime)) {
		render_html(cfg, body, lines, links, base, true);
	} else {
		render_plaintext(body, lines, OutputFormat::PlainText);
	}

	render_links_summary(lines, links);

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
	RssItem& item,
	unsigned int text_width,
	unsigned int window_width,
	RegexManager* rxman,
	const std::string& location,
	Links& links)
{
	std::vector<std::pair<LineType, std::string>> lines;
	const auto item_description = item.description();

	prepare_header(item, lines, links);
	prepare_enclosure(item, lines, links, false);

	const std::string baseurl = get_item_base_link(item);
	const auto body = utils::utf8_to_locale(item_description.text);

	if (should_render_as_html(item_description.mime)) {
		render_html(cfg, body, lines, links, baseurl, false);
	} else {
		render_plaintext(body, lines, OutputFormat::StflRichText);
	}

	render_links_summary(lines, links);

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
	RssItem& item,
	unsigned int text_width,
	unsigned int window_width,
	RegexManager* rxman,
	const std::string& location)
{
	std::vector<std::pair<LineType, std::string>> lines;
	Links links;

	prepare_header(item, lines, links);
	render_source(lines, utils::quote_for_stfl(utils::utf8_to_locale(
				item.description().text)));

	TextFormatter txtfmt;
	txtfmt.add_lines(lines);

	return txtfmt.format_text_to_list(rxman, location, text_width, window_width);
}

}

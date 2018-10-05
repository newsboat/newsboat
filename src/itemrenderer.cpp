#include "itemrenderer.h"

#include "configcontainer.h"
#include "htmlrenderer.h"
#include "rss.h"
#include "textformatter.h"

namespace newsboat {

ItemRenderer::ItemRenderer(ConfigContainer* cfg_)
	: cfg(cfg_)
{}

std::string ItemRenderer::to_plain_text(std::shared_ptr<RssItem> item) {

	std::vector<std::pair<LineType, std::string>> lines;

	if (!item->title().empty()) {
		std::string title(_("Title: "));
		title.append(item->title());
		lines.push_back(std::make_pair(LineType::wrappable, title));
	}

	if (!item->author().empty()) {
		std::string author(_("Author: "));
		author.append(item->author());
		lines.push_back(std::make_pair(LineType::wrappable, author));
	}

	std::string date(_("Date: "));
	date.append(item->pubDate());
	lines.push_back(std::make_pair(LineType::wrappable, date));

	if (!item->link().empty()) {
		std::string link(_("Link: "));
		link.append(item->link());
		lines.push_back(std::make_pair(LineType::softwrappable, link));
	}

	if (!item->enclosure_url().empty()) {
		std::string dlurl(_("Podcast Download URL: "));
		dlurl.append(item->enclosure_url());
		lines.push_back(std::make_pair(LineType::softwrappable, dlurl));
	}

	lines.push_back(std::make_pair(LineType::wrappable, std::string("")));

	HtmlRenderer rnd(true);
	std::vector<LinkPair> links; // not used
	rnd.render(item->description(), lines, links, item->feedurl());
	TextFormatter txtfmt;
	txtfmt.add_lines(lines);

	unsigned int width = cfg->get_configvalue_as_int("text-width");
	if (width == 0) {
		width = 80;
	}

	return txtfmt.format_text_plain(width);
}

}

#include "itemrenderer.h"

#include "configcontainer.h"
#include "htmlrenderer.h"
#include "rss.h"
#include "textformatter.h"

namespace newsboat {

std::string get_feedtitle(std::shared_ptr<RssItem> item) {
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

void prepare_header(
	std::shared_ptr<RssItem> item,
	std::vector<std::pair<LineType, std::string>>& lines,
	std::vector<LinkPair>& /*links*/)
{
	const auto feedtitle = get_feedtitle(item);
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

std::string item_renderer::to_plain_text(
		ConfigContainer& cfg,
		std::shared_ptr<RssItem> item)
{
	std::vector<std::pair<LineType, std::string>> lines;
	std::vector<LinkPair> links;

	prepare_header(item, lines, links);

	HtmlRenderer rnd(true);
	rnd.render(item->description(), lines, links, item->feedurl());

	TextFormatter txtfmt;
	txtfmt.add_lines(lines);

	unsigned int width = cfg.get_configvalue_as_int("text-width");
	if (width == 0) {
		width = 80;
	}

	return txtfmt.format_text_plain(width);
}

}

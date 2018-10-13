#ifndef NEWSBOAT_ITEMRENDERER_H_
#define NEWSBOAT_ITEMRENDERER_H_

#include <memory>
#include <string>
#include <vector>

#include "textformatter.h"
#include "htmlrenderer.h"

namespace newsboat {

class ConfigContainer;
class RssItem;

namespace item_renderer {
	std::string get_feedtitle(std::shared_ptr<RssItem> item);

	std::string to_plain_text(
			ConfigContainer& cfg,
			std::shared_ptr<RssItem> item);

	std::string get_item_base_link(const std::shared_ptr<RssItem>& item);

	void render_html(
		ConfigContainer& cfg,
		const std::string& source,
		std::vector<std::pair<LineType, std::string>>& lines,
		std::vector<LinkPair>& thelinks,
		const std::string& url,
		bool raw);
};

} // namespace newsboat

#endif /* NEWSBOAT_ITEMRENDERER_H_ */

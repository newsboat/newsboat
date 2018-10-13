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

	std::pair<std::string, size_t> to_stfl_list(
			ConfigContainer& cfg,
			std::shared_ptr<RssItem> item,
			unsigned int text_width,
			unsigned int window_width,
			RegexManager* rxman,
			const std::string& location);

	std::pair<std::string, size_t> source_to_stfl_list(
			std::shared_ptr<RssItem> item,
			unsigned int text_width,
			unsigned int window_width,
			RegexManager* rxman,
			const std::string& location);
};

} // namespace newsboat

#endif /* NEWSBOAT_ITEMRENDERER_H_ */

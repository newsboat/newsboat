#ifndef NEWSBOAT_ITEMRENDERER_H_
#define NEWSBOAT_ITEMRENDERER_H_

#include <memory>
#include <string>

namespace newsboat {

class ConfigContainer;
class RssItem;

class ItemRenderer {
public:
	ItemRenderer() = default;

	std::string to_plain_text(
			ConfigContainer& cfg,
			std::shared_ptr<RssItem> item);
};

} // namespace newsboat

#endif /* NEWSBOAT_ITEMRENDERER_H_ */

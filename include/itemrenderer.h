#ifndef NEWSBOAT_ITEMRENDERER_H_
#define NEWSBOAT_ITEMRENDERER_H_

#include <memory>
#include <string>
#include <vector>

#include "htmlrenderer.h"
#include "textformatter.h"

namespace newsboat {

class ConfigContainer;
class RssItem;

/// \brief Creates different textual representations of an RssItem.
namespace item_renderer {
	/// \brief Returns feed's title of the item, or some replacement value if
	/// the title isn't available.
	std::string get_feedtitle(std::shared_ptr<RssItem> item);

	/// \brief Returns plain-text representation of the RssItem.
	///
	/// Markup is mostly stripped. Links are numbered and tagged. The text is
	/// wrapped at the width specified by the `text-width` setting.
	std::string to_plain_text(
			ConfigContainer& cfg,
			std::shared_ptr<RssItem> item);

	/// \brief Returns RssItem as STFL list.
	///
	/// `html-renderer` settings controls what tool is used to render HTML. \a
	/// text_width dictates where text is wrapped. \a window_width dictates
	/// where URLs are wrapped. \a rxman rules for \a location are used to
	/// highlight the resulting text.
	std::pair<std::string, size_t> to_stfl_list(
			ConfigContainer& cfg,
			std::shared_ptr<RssItem> item,
			unsigned int text_width,
			unsigned int window_width,
			RegexManager* rxman,
			const std::string& location);

	/// \brief Returns RssItem's text source as STFL list.
	///
	/// \a text_width dictates where text is wrapped. \a window_width dictates
	/// where URLs are wrapped. \a rxman rules for \a location are used to
	/// highlight the resulting text.
	std::pair<std::string, size_t> source_to_stfl_list(
			std::shared_ptr<RssItem> item,
			unsigned int text_width,
			unsigned int window_width,
			RegexManager* rxman,
			const std::string& location);
};

} // namespace newsboat

#endif /* NEWSBOAT_ITEMRENDERER_H_ */

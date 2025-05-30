#ifndef NEWSBOAT_ITEMRENDERER_H_
#define NEWSBOAT_ITEMRENDERER_H_

#include <string>
#include <vector>

#include "dialog.h"
#include "links.h"
#include "textformatter.h"

namespace newsboat {

class ConfigContainer;
class RssItem;

/// \brief Creates different textual representations of an RssItem.
namespace item_renderer {

enum class OutputFormat {
	PlainText,
	StflRichText,
};

/// \brief Returns feed's title of the item, or some replacement value if
/// the title isn't available.
std::string get_feedtitle(RssItem& item);

/// \brief Splits text into lines marked as wrappable
void render_plaintext(const std::string& source,
	std::vector<std::pair<LineType, std::string>>& lines, OutputFormat format);

/// \brief Returns plain-text representation of the RssItem.
///
/// Markup is mostly stripped. Links are numbered and tagged. The text is
/// wrapped at the width specified by the `text-width` setting.
std::string to_plain_text(
	ConfigContainer& cfg,
	RssItem& item);

/// \brief Returns RssItem as STFL list.
///
/// `html-renderer` settings controls what tool is used to render HTML. \a
/// text_width dictates where text is wrapped. \a window_width dictates
/// where URLs are wrapped. \a rxman rules for \a location are used to
/// highlight the resulting text. \a links is filled with all the links
/// found in the article (which is useful for article view, which lets
/// users open the links by their number).
std::pair<std::string, size_t> to_stfl_list(
	ConfigContainer& cfg,
	RssItem& item,
	unsigned int text_width,
	unsigned int window_width,
	RegexManager* rxman,
	Dialog location,
	Links& links);

/// \brief Returns RssItem's text source as STFL list.
///
/// \a text_width dictates where text is wrapped. \a window_width dictates
/// where URLs are wrapped. \a rxman rules for \a location are used to
/// highlight the resulting text.
std::pair<std::string, size_t> source_to_stfl_list(
	RssItem& item,
	unsigned int text_width,
	unsigned int window_width,
	RegexManager* rxman,
	Dialog location);
};

} // namespace newsboat

#endif /* NEWSBOAT_ITEMRENDERER_H_ */

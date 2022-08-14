#ifndef NEWSBOAT_HTMLRENDERER_H_
#define NEWSBOAT_HTMLRENDERER_H_

#include "libnewsboat-ffi/src/htmlrenderer.rs.h"

#include <string>
#include <vector>

#include "textformatter.h"

namespace newsboat {

enum class LinkType {
	HREF,
	IMG,
	EMBED,
	IFRAME,
	VIDEO,
	AUDIO,
};
typedef std::pair<std::string, LinkType> LinkPair;

class HtmlRenderer {
public:
	explicit HtmlRenderer(bool raw = false);
	void render(const std::string& source,
		std::vector<std::pair<LineType, std::string>>& lines,
		std::vector<LinkPair>& links,
		const std::string& url);
	static std::string render_hr(unsigned int width);
private:
	rust::Box<htmlrenderer::bridged::HtmlRenderer> rs_object;
};

} // namespace newsboat

#endif /* NEWSBOAT_HTMLRENDERER_H_ */

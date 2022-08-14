#include "htmlrenderer.h"

namespace newsboat {

HtmlRenderer::HtmlRenderer(bool raw)
	: rs_object(htmlrenderer::bridged::create(raw))
{
}

void HtmlRenderer::render(const std::string& source,
	std::vector<std::pair<LineType, std::string>>& lines,
	std::vector<LinkPair>& links,
	const std::string& url)
{
	rust::Vec<htmlrenderer::bridged::LineType> line_types;
	rust::Vec<rust::String> line_content;
	rust::Vec<htmlrenderer::bridged::LinkType> link_types;
	rust::Vec<rust::String> link_content;
	htmlrenderer::bridged::render(*rs_object, source, line_types, line_content, link_types,
		link_content, url);

	for (size_t i = 0; i < line_content.size(); ++i) {
		lines.emplace_back((LineType)line_types[i], std::string(line_content[i]));
	}

	for (size_t i = 0; i < link_content.size(); ++i) {
		links.emplace_back(std::string(link_content[i]), (LinkType)link_types[i]);
	}
}

std::string HtmlRenderer::render_hr(unsigned int width)
{
	std::string result = "\n ";
	result += std::string(width - 2, '-');
	result += " \n";

	return result;
}

} // namespace newsboat

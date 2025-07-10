#include "charencoding.h"

#include "libNewsboat-ffi/src/charencoding.rs.h"

namespace Newsboat {
namespace charencoding {

std::optional<std::string> charset_from_bom(std::vector<std::uint8_t> content)
{
	rust::String charset;
	const auto input = rust::Slice<const std::uint8_t>(content.data(), content.size());
	if (charencoding::bridged::charset_from_bom(input, charset)) {
		return std::string(charset);
	}
	return {};
}

std::optional<std::string> charset_from_xml_declaration(std::vector<std::uint8_t>
	content)
{
	rust::String charset;
	const auto input = rust::Slice<const std::uint8_t>(content.data(), content.size());
	if (charencoding::bridged::charset_from_xml_declaration(input, charset)) {
		return std::string(charset);
	}
	return {};
}

std::optional<std::string> charset_from_content_type_header(std::vector<std::uint8_t>
	header)
{
	rust::String charset;
	const auto input = rust::Slice<const std::uint8_t>(header.data(), header.size());
	if (charencoding::bridged::charset_from_content_type_header(input, charset)) {
		return std::string(charset);
	}
	return {};
}

} // namespace charencoding
} // namespace Newsboat

#include "charencoding.h"

#include "libnewsboat-ffi/src/charencoding.rs.h"

namespace newsboat {
namespace charencoding {

nonstd::optional<std::string> charset_from_bom(std::vector<std::uint8_t> content)
{
	rust::String charset;
	const auto input = rust::Slice<const std::uint8_t>(content.data(), content.size());
	if (charencoding::bridged::charset_from_bom(input, charset)) {
		return std::string(charset);
	}
	return {};
}

nonstd::optional<std::string> charset_from_xml_declaration(std::vector<std::uint8_t>
	content)
{
	rust::String charset;
	const auto input = rust::Slice<const std::uint8_t>(content.data(), content.size());
	if (charencoding::bridged::charset_from_xml_declaration(input, charset)) {
		return std::string(charset);
	}
	return {};
}

} // namespace charencoding
} // namespace newsboat

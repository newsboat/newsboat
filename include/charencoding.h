#ifndef NEWSBOAT_CHARENCODING_H_
#define NEWSBOAT_CHARENCODING_H_

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace Newsboat {
namespace charencoding {

std::optional<std::string> charset_from_bom(std::vector<std::uint8_t> content);
std::optional<std::string> charset_from_xml_declaration(std::vector<std::uint8_t>
	content);
std::optional<std::string> charset_from_content_type_header(std::vector<std::uint8_t>
	header);

} // namespace charencoding
} // namespace Newsboat

#endif /* NEWSBOAT_CHARENCODING_H_ */

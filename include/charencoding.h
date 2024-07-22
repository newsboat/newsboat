#ifndef NEWSBOAT_CHARENCODING_H_
#define NEWSBOAT_CHARENCODING_H_

#include <cstdint>
#include <string>
#include <vector>

#include "3rd-party/optional.hpp"

namespace newsboat {
namespace charencoding {

nonstd::optional<std::string> charset_from_bom(std::vector<std::uint8_t> content);
nonstd::optional<std::string> charset_from_xml_declaration(std::vector<std::uint8_t>
	content);
nonstd::optional<std::string> charset_from_content_type_header(std::vector<std::uint8_t>
	header);

} // namespace charencoding
} // namespace newsboat

#endif /* NEWSBOAT_CHARENCODING_H_ */

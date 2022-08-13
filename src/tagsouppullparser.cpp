#include "tagsouppullparser.h"

#include <string>

namespace newsboat {

/*
 * This method implements an "XML" pull parser. In reality, it's more liberal
 * than any XML pull parser, as it basically accepts everything that even only
 * remotely looks like XML. We use this parser for the HTML renderer.
 */

TagSoupPullParser::TagSoupPullParser(std::string s)
	: rs_object(tagsouppullparser::bridged::create(s))
{
}

nonstd::optional<std::string> TagSoupPullParser::get_attribute_value(
	const std::string& name) const
{
	rust::String value;
	if (tagsouppullparser::bridged::get_attribute_value(*rs_object, name, value)) {
		return std::string(value);
	}
	return nonstd::nullopt;
}

std::string TagSoupPullParser::get_text() const
{
	return std::string(tagsouppullparser::bridged::get_text(*rs_object));
}

TagSoupPullParser::Event TagSoupPullParser::next()
{
	return tagsouppullparser::bridged::next(*rs_object);
}

} // namespace newsboat

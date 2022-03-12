#include "tagsouppullparser.h"

#include <algorithm>
#include <cinttypes>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <iostream>
#include <istream>
#include <sstream>
#include <stdexcept>

#include "config.h"
#include "logger.h"
#include "utils.h"

namespace newsboat {

TagSoupPullParser::TagSoupPullParser(std::istream& is)
	: TagSoupPullParser(std::string(std::istreambuf_iterator<char>(is), {}))
{
}

TagSoupPullParser::TagSoupPullParser(const std::string& s)
	: rs_object(tagsouppullparser::bridged::create(s))
{
}

TagSoupPullParser::~TagSoupPullParser() {}

nonstd::optional<std::string> TagSoupPullParser::get_attribute_value(
	const std::string& name) const
{
	rust::String value;
	if (newsboat::tagsouppullparser::bridged::get_attribute_value(*rs_object, name, value)) {
		return std::string(value);
	}
	return nonstd::nullopt;
}

TagSoupPullParser::Event TagSoupPullParser::get_event_type() const
{
	return newsboat::tagsouppullparser::bridged::get_event_type(*rs_object);
}

std::string TagSoupPullParser::get_text() const
{
	return std::string(newsboat::tagsouppullparser::bridged::get_text(*rs_object));
}

TagSoupPullParser::Event TagSoupPullParser::next()
{
	return newsboat::tagsouppullparser::bridged::next(*rs_object);
}

} // namespace newsboat

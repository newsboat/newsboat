#ifndef NEWSBOAT_TAGSOUPPULLPARSER_H_
#define NEWSBOAT_TAGSOUPPULLPARSER_H_

#include "libnewsboat-ffi/src/tagsouppullparser.rs.h"

#include <string>

#include "3rd-party/optional.hpp"

namespace newsboat {

class TagSoupPullParser {
public:
	using Event = tagsouppullparser::bridged::Event;

	TagSoupPullParser(std::istream& is);
	TagSoupPullParser(const std::string& input);
	virtual ~TagSoupPullParser();
	nonstd::optional<std::string> get_attribute_value(const std::string& name) const;
	Event get_event_type() const;
	std::string get_text() const;
	Event next();

private:
	rust::Box<tagsouppullparser::bridged::TagSoupPullParser> rs_object;
};

} // namespace newsboat

#endif /* NEWSBOAT_TAGSOUPPULLPARSER_H_ */

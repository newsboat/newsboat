#ifndef NEWSBOAT_TAGSOUPPULLPARSER_H_
#define NEWSBOAT_TAGSOUPPULLPARSER_H_

#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "3rd-party/optional.hpp"

namespace newsboat {

class TagSoupPullParser {
public:
	enum class Event {
		START_DOCUMENT,
		END_DOCUMENT,
		START_TAG,
		END_TAG,
		TEXT
	};

	TagSoupPullParser(std::string s);
	~TagSoupPullParser() = default;
	nonstd::optional<std::string> get_attribute_value(const std::string& name) const;
	Event get_event_type() const;
	std::string get_text() const;
	Event next();


private:
	std::istringstream inputstream;

	typedef std::pair<std::string, std::string> Attribute;
	std::vector<Attribute> attributes;
	std::string text;
	Event current_event;

	void add_attribute(std::string s);
	nonstd::optional<std::string> read_tag();
	Event determine_tag_type();
	std::string decode_attribute(const std::string& s);
	std::string decode_entities(const std::string& s);
	std::string decode_entity(std::string s);
	void parse_tag(const std::string& tagstr);
	void handle_tag();
	void handle_text(char c);
};

} // namespace newsboat

#endif /* NEWSBOAT_TAGSOUPPULLPARSER_H_ */

#ifndef NEWSBOAT_TAGSOUPPULLPARSER_H_
#define NEWSBOAT_TAGSOUPPULLPARSER_H_

#include <string>
#include <utility>
#include <vector>

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

	TagSoupPullParser(std::istream& is);
	virtual ~TagSoupPullParser();
	void set_input(std::istream& is);
	std::string get_attribute_value(const std::string& name) const;
	Event get_event_type() const;
	std::string get_text() const;
	Event next();

private:
	std::istream& inputstream;

	typedef std::pair<std::string, std::string> Attribute;
	std::vector<Attribute> attributes;
	std::string text;
	Event current_event;

	void add_attribute(std::string s);
	std::string read_tag();
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

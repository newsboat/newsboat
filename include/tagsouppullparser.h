#ifndef NEWSBOAT_TAGSOUPPULLPARSER_H_
#define NEWSBOAT_TAGSOUPPULLPARSER_H_

#include <string>
#include <utility>
#include <vector>

namespace newsboat {

class tagsouppullparser {
public:
	enum class event {
		START_DOCUMENT,
		END_DOCUMENT,
		START_TAG,
		END_TAG,
		TEXT
	};

	tagsouppullparser();
	virtual ~tagsouppullparser();
	void set_input(std::istream& is);
	std::string get_attribute_value(const std::string& name) const;
	event get_event_type() const;
	std::string get_text() const;
	event next();

private:
	typedef std::pair<std::string, std::string> attribute;
	std::vector<attribute> attributes;
	std::string text;
	std::istream* inputstream;
	event current_event;

	void skip_whitespace();
	void add_attribute(std::string s);
	std::string read_tag();
	event determine_tag_type();
	std::string decode_attribute(const std::string& s);
	std::string decode_entities(const std::string& s);
	std::string decode_entity(std::string s);
	void parse_tag(const std::string& tagstr);
	void handle_tag();
	void handle_text();

	std::string ws;
	char c;
};

} // namespace newsboat

#endif /* NEWSBOAT_TAGSOUPPULLPARSER_H_ */

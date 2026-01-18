#ifndef NEWSBOAT_TEXTSTYLE_H_
#define NEWSBOAT_TEXTSTYLE_H_

#include <string>
#include <vector>

namespace newsboat {

class TextStyle {
public:
	TextStyle(const std::string& fgcolor, const std::string& bgcolor,
		const std::vector<std::string>& attributes);

	const std::string& get_fgcolor() const;
	const std::string& get_bgcolor() const;
	const std::vector<std::string>& get_attributes() const;

	std::string get_stfl_style_string() const;

private:
	std::string fgcolor;
	std::string bgcolor;
	std::vector<std::string> attributes;
};

} // namespace newsboat

#endif /* NEWSBOAT_TEXTSTYLE_H_ */

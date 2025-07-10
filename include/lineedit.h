#ifndef NEWSBOAT_LINEEDIT_H_
#define NEWSBOAT_LINEEDIT_H_

#include "stflpp.h"

#include <cstdint>

namespace Newsboat {

class LineEdit {
public:
	LineEdit(Stfl::Form& form, const std::string& name);

	void set_value(const std::string& value);
	std::string get_value();

	void show();
	void hide();

	void set_position(std::uint32_t pos);
	std::uint32_t get_position();

private:
	Stfl::Form& f;
	const std::string name;
};

} // namespace Newsboat

#endif /* NEWSBOAT_LINEEDIT_H_ */

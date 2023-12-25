#ifndef NEWSBOAT_LINEVIEW_H_
#define NEWSBOAT_LINEVIEW_H_

#include <string>

#include "stflpp.h"

namespace newsboat {

class LineView {
public:
	LineView(Stfl::Form& form, const std::string& name);

	void set_text(const std::string& text);
	std::string get_text();

	void show();
	void hide();

private:
	Stfl::Form& f;
	const std::string name;
};

} // namespace newsboat

#endif /* NEWSBOAT_LINEVIEW_H_ */

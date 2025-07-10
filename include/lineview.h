#ifndef NEWSBOAT_LINEVIEW_H_
#define NEWSBOAT_LINEVIEW_H_

#include <string>

#include "stflpp.h"

namespace Newsboat {

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

} // namespace Newsboat

#endif /* NEWSBOAT_LINEVIEW_H_ */

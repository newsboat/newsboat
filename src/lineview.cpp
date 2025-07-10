#include "lineview.h"

namespace Newsboat {

LineView::LineView(Stfl::Form& form, const std::string& name)
	: f(form)
	, name(name)
{
}

void LineView::set_text(const std::string& text)
{
	f.set(name, text);
}

std::string LineView::get_text()
{
	return f.get(name);
}

void LineView::show()
{
	f.set("show_" + name, "1");
}

void LineView::hide()
{
	f.set("show_" + name, "0");
}


} // namespace Newsboat

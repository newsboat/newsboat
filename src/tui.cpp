#include "tui.h"

#include <cassert>

#include "libnewsboat-ffi/src/tui.rs.h"

namespace {
// TODO: Remove
newsboat::Tui* tui_instance = nullptr;
}

namespace newsboat {

RustForm::RustForm()
	: rs_object(tui::bridged::create_form())
{
}

std::string RustForm::get_variable(std::string key)
{
	return std::string(tui::bridged::get_variable(*rs_object, key));
}

void RustForm::set_variable(std::string key, std::string value)
{
	tui::bridged::set_variable(*rs_object, key, value);
}

void RustForm::modify_form(std::string name, std::string mode, std::string value)
{
	tui::bridged::modify_form(*rs_object, name, mode, value);
}

Tui::Tui()
	: rs_object(tui::bridged::create())
{
	if (tui_instance == nullptr) {
		tui_instance = this;
	}
}

Tui::~Tui()
{
	//tui_instance = nullptr;
}

Tui& Tui::get_instance()
{
	assert(tui_instance != nullptr);
	return *tui_instance;
}

void Tui::reset()
{
	tui::bridged::reset(*rs_object);
}

std::string Tui::run(RustForm& form, std::int32_t timeout)
{
	return std::string(tui::bridged::run(*rs_object, *form.rs_object, timeout));
}

} // namespace newsboat

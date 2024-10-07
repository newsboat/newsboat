#include "tui.h"

#include <cassert>

#include "libnewsboat-ffi/src/tui.rs.h"

namespace {
newsboat::Tui* tui_instance = nullptr;
}

namespace newsboat {

Tui::Tui()
	: rs_object(tui::bridged::create())
{
	tui_instance = this;
}

Tui::~Tui()
{
	tui_instance = nullptr;
}

Tui& Tui::get_instance()
{
	assert(tui_instance != nullptr);
	return *tui_instance;
}

std::string Tui::run(std::int32_t timeout)
{
	return std::string(tui::bridged::run(*rs_object, timeout));
}

std::string Tui::get_variable(std::string key)
{
	return std::string(tui::bridged::get_variable(*rs_object, key));
}

void Tui::set_variable(std::string key, std::string value)
{
	tui::bridged::set_variable(*rs_object, key, value);
}

void Tui::modify_form(std::string name, std::string mode, std::string value)
{
	tui::bridged::modify_form(*rs_object, name, mode, value);
}

} // namespace newsboat

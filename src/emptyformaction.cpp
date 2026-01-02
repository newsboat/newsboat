#include "emptyformaction.h"

namespace newsboat {

EmptyFormAction::EmptyFormAction(View& v, const std::string& formstr,
	ConfigContainer* cfg)
	: FormAction(v, formstr, cfg)
{
}

Dialog EmptyFormAction::id() const
{
	return Dialog::Empty;
}

std::string EmptyFormAction::title()
{
	return "Empty FormAction";
}

void EmptyFormAction::init()
{
}

void EmptyFormAction::prepare()
{
}

std::vector<KeyMapHintEntry> EmptyFormAction::get_keymap_hint() const
{
	static const std::vector<KeyMapHintEntry> hints;
	return hints;
}

bool EmptyFormAction::process_operation(Operation /*op*/,
	const std::vector<std::string>& /*args*/,
	BindingType /*bindingType*/)
{
	return false;
}

} // namespace newsboat

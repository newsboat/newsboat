#include "emptyformaction.h"

namespace newsboat {

EmptyFormAction::EmptyFormAction(View* v, const std::string& formstr,
	ConfigContainer* cfg)
	: FormAction(v, formstr, cfg)
{
}

std::string EmptyFormAction::id() const
{
	return "empty";
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

KeyMapHintEntry* EmptyFormAction::get_keymap_hint()
{
	static KeyMapHintEntry hints[] = {
		{OP_NIL, nullptr}
	};
	return hints;
}

bool EmptyFormAction::process_operation(Operation /*op*/,
	bool /*automatic*/,
	std::vector<std::string>* /*args*/)
{
	return false;
}

} // namespace newsboat

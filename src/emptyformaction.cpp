#include "emptyformaction.h"

namespace newsboat {

EmptyFormAction::EmptyFormAction(View* v, const Utf8String& formstr,
	ConfigContainer* cfg)
	: FormAction(v, formstr, cfg)
{
}

Utf8String EmptyFormAction::id() const
{
	return "empty";
}

Utf8String EmptyFormAction::title()
{
	return "Empty FormAction";
}

void EmptyFormAction::init()
{
}

void EmptyFormAction::prepare()
{
}

const std::vector<KeyMapHintEntry>& EmptyFormAction::get_keymap_hint() const
{
	static const std::vector<KeyMapHintEntry> hints;
	return hints;
}

bool EmptyFormAction::process_operation(Operation /*op*/,
	bool /*automatic*/,
	std::vector<Utf8String>* /*args*/)
{
	return false;
}

} // namespace newsboat

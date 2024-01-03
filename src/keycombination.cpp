#include "keycombination.h"

#include <locale>
#include <tuple>

namespace newsboat {

KeyCombination::KeyCombination(const std::string& key, ShiftState shift,
	ControlState control, AltState alt)
	: key(key)
	, shift(shift)
	, control(control)
	, alt(alt)
{
}

KeyCombination KeyCombination::from_bindkey(const std::string& input)
{
	std::string key = input;
	ShiftState shift = ShiftState::NoShift;
	ControlState control = ControlState::NoControl;
	AltState alt = AltState::NoAlt;

	if (key.length() == 1 && std::isupper(key[0])) {
		shift = ShiftState::Shift;
		key = std::tolower(key[0]);
	} else if (key.length() == 2 && key[0] == '^') {
		control = ControlState::Control;
		key = std::tolower(key[1]);
	}
	return KeyCombination(key, shift, control, alt);
}

std::string KeyCombination::to_bindkey_string() const
{
	if (control == ControlState::Control && key.length() == 1) {
		return std::string("^") + static_cast<char>(std::toupper(key[0]));
	} else if (shift == ShiftState::Shift && key.length() == 1) {
		return std::string{static_cast<char>(std::toupper(key[0]))};
	} else {
		return key;
	}
}

bool KeyCombination::operator==(const KeyCombination& other) const
{
	return key == other.key
		&& shift == other.shift
		&& control == other.control
		&& alt == other.alt;
}

bool KeyCombination::operator<(const KeyCombination& rhs) const
{
	return std::tie(key, shift, control, alt)
		< std::tie(rhs.key, rhs.shift, rhs.control, rhs.alt);
}

std::string KeyCombination::get_key() const
{
	return key;
}

bool KeyCombination::has_shift() const
{
	return shift == ShiftState::Shift;
}

bool KeyCombination::has_control() const
{
	return control == ControlState::Control;
}

bool KeyCombination::has_alt() const
{
	return alt == AltState::Alt;
}

} // namespace newsboat

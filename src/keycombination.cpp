#include "keycombination.h"

#include <tuple>

#include "libnewsboat-ffi/src/keycombination.rs.h"

namespace newsboat {

KeyCombination convert(const keycombination::bridged::KeyCombination& key_combination)
{
	ShiftState shift = ShiftState::NoShift;
	ControlState control = ControlState::NoControl;
	AltState alt = AltState::NoAlt;

	const auto key = std::string(keycombination::bridged::get_key(key_combination));
	if (keycombination::bridged::has_shift(key_combination)) {
		shift = ShiftState::Shift;
	}
	if (keycombination::bridged::has_control(key_combination)) {
		control = ControlState::Control;
	}
	if (keycombination::bridged::has_alt(key_combination)) {
		alt = AltState::Alt;
	}

	return KeyCombination(key, shift, control, alt);
}

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
	const auto key_combination = keycombination::bridged::from_bindkey(input);
	return convert(*key_combination);
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

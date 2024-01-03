#include "keycombination.h"

#include <locale>
#include <tuple>

namespace newsboat {

KeyCombination::KeyCombination(const std::string& key, bool shift, bool control, bool alt)
	: key(key)
	, shift(shift)
	, control(control)
	, alt(alt)
{
}

KeyCombination KeyCombination::from_bindkey(const std::string& input)
{
	std::string key = input;
	bool shift = false;
	bool control = false;
	bool alt = false;

	if (key.length() == 1 && std::isupper(key[0])) {
		shift = true;
		key = std::tolower(key[0]);
	} else if (key.length() == 2 && key[0] == '^') {
		control = true;
		key = std::tolower(key[1]);
	}
	return KeyCombination(key, shift, control, alt);
}

std::string KeyCombination::to_bindkey_string() const
{
	if (control && key.length() == 1) {
		return std::string("^") + static_cast<char>(std::toupper(key[0]));
	}
	else if (shift && key.length() == 1) {
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
	return shift;
}

bool KeyCombination::has_control() const
{
	return control;
}

bool KeyCombination::has_alt() const
{
	return alt;
}

} // namespace newsboat

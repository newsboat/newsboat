#include "keycombination.h"

#include <algorithm>
#include <cctype>
#include <iterator>
#include <locale>
#include <tuple>

#include "libnewsboat-ffi/src/keycombination.rs.h"
#include "strprintf.h"

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

std::vector<KeyCombination> KeyCombination::from_bind(const std::string& input)
{
	std::vector<KeyCombination> key_combinations;
	auto cur = input.begin();
	while (cur != input.end()) {
		if (*cur == '<') {
			std::advance(cur, 1);
			const auto close_location = std::find(cur, input.end(), '>');
			if (close_location != input.end()) {
				ShiftState shift = ShiftState::NoShift;
				ControlState control = ControlState::NoControl;
				AltState alt = AltState::NoAlt;
				auto key = std::string(cur, close_location);

				if (key.find("C-") == 0) {
					key = key.substr(2);
					control = ControlState::Control;
				}

				if (key.find("S-") == 0) {
					key = key.substr(2);
					shift = ShiftState::Shift;
				}

				if (key.find("M-") == 0) {
					key = key.substr(2);
					alt = AltState::Alt;
				}

				key_combinations.push_back(KeyCombination(key, shift, control, alt));
				cur = std::next(close_location);
				continue;
			} else {
				// TODO: Report error?
				break;
			}
		}
		if (std::isupper(*cur)) {
			const auto key = std::string(1, static_cast<char>(std::tolower(*cur)));
			key_combinations.push_back(KeyCombination(key, ShiftState::Shift));
			std::advance(cur, 1);
			continue;
		}
		if (std::distance(cur, input.end()) >= 2 && *cur == '^' && std::isupper(*std::next(cur))) {
			const auto key = std::string(1, static_cast<char>(std::tolower(*std::next(cur))));
			key_combinations.push_back(KeyCombination(key, ShiftState::NoShift,
					ControlState::Control));
			std::advance(cur, 2);
			continue;
		}
		const auto key = std::string(cur, std::next(cur));
		key_combinations.push_back(KeyCombination(key));
		std::advance(cur, 1);
	}
	return key_combinations;
}

std::string KeyCombination::to_bindkey_string() const
{
	if (control == ControlState::Control && key.length() == 1) {
		return strprintf::fmt("^%c", static_cast<char>(std::toupper(key[0])));
	} else if (shift == ShiftState::Shift && key.length() == 1) {
		return strprintf::fmt("%c", static_cast<char>(std::toupper(key[0])));
	} else {
		return key;
	}
}

std::string KeyCombination::to_bind_string() const
{
	if (key.size() == 1 && isalpha(key[0])) {
		if (has_shift() && !has_control() && !has_alt()) {
			return strprintf::fmt("%c", static_cast<char>(std::toupper(key[0])));
		}
		if (!has_shift() && has_control() && !has_alt()) {
			return strprintf::fmt("^%c", static_cast<char>(std::toupper(key[0])));
		}
	}
	if (key.size() == 1 && !has_shift() && !has_control() && !has_alt()) {
		return key;
	}

	std::string modifiers = "";
	if (has_control()) {
		modifiers += "C-";
	}
	if (has_shift()) {
		modifiers += "S-";
	}
	if (has_alt()) {
		modifiers += "M-";
	}
	return strprintf::fmt("<%s%s>", modifiers, key);
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

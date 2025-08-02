#ifndef NEWSBOAT_KEYCOMBINATION_H_
#define NEWSBOAT_KEYCOMBINATION_H_

#include <string>
#include <vector>

namespace newsboat {

enum class ShiftState {
	Shift,
	NoShift,
};

enum class ControlState {
	Control,
	NoControl,
};

enum class AltState {
	Alt,
	NoAlt,
};

class KeyCombination {
public:
	explicit KeyCombination(const std::string& key,
		ShiftState shift = ShiftState::NoShift,
		ControlState control = ControlState::NoControl,
		AltState alt = AltState::NoAlt);

	static KeyCombination from_bindkey(const std::string& input);
	static std::vector<KeyCombination> from_bind(const std::string& input);
	std::string to_bindkey_string() const;
	std::string to_bind_string() const;

	bool operator==(const KeyCombination& other) const;
	bool operator<(const KeyCombination& rhs) const;

	const std::string& get_key() const;
	bool has_shift() const;
	bool has_control() const;
	bool has_alt() const;

private:
	std::string key;
	ShiftState shift;
	ControlState control;
	AltState alt;
};

} // namespace newsboat

#endif /* NEWSBOAT_KEYCOMBINATION_H_ */

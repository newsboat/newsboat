#ifndef NEWSBOAT_KEYCOMBINATION_H_
#define NEWSBOAT_KEYCOMBINATION_H_

#include <ostream>
#include <string>

namespace newsboat {

class KeyCombination {
public:
	KeyCombination(const std::string& key, bool shift, bool control, bool alt);

	static KeyCombination from_bindkey(const std::string& input);
	std::string to_bindkey_string() const;

	bool operator==(const KeyCombination& other) const;
	bool operator<(const KeyCombination& rhs) const;

	std::string get_key() const;
	bool has_shift() const;
	bool has_control() const;
	bool has_alt() const;

private:
	std::string key;
	bool shift;
	bool control;
	bool alt;
};

} // namespace newsboat

#endif /* NEWSBOAT_KEYCOMBINATION_H_ */

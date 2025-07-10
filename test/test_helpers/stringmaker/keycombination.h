#ifndef NEWSBOAT_TEST_HELPERS_STRINGMAKER_KEYCOMBINATION_H_
#define NEWSBOAT_TEST_HELPERS_STRINGMAKER_KEYCOMBINATION_H_

#include "keycombination.h"
#include "3rd-party/catch.hpp"

namespace Catch {
template<>
struct StringMaker<Newsboat::KeyCombination> {
	static std::string convert(const Newsboat::KeyCombination& key_combination)
	{
		return std::string("<")
			+ (key_combination.has_control() ? "C-" : "")
			+ (key_combination.has_shift() ? "S-" : "")
			+ (key_combination.has_alt() ? "M-" : "")
			+ key_combination.get_key()
			+ ">";
	}
};
}

#endif /* NEWSBOAT_TEST_HELPERS_STRINGMAKER_KEYCOMBINATION_H_ */

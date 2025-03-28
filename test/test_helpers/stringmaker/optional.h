#ifndef NEWSBOAT_TEST_HELPERS_STRINGMAKER_OPTIONAL_H_
#define NEWSBOAT_TEST_HELPERS_STRINGMAKER_OPTIONAL_H_

#include <optional>

#include "3rd-party/catch.hpp"

namespace Catch {
template<typename T>
struct StringMaker<std::optional<T>> {
	static std::string convert(const std::optional<T>& opt)
	{
		if (opt.has_value()) {
			return std::string("std::optional( ")
				+ StringMaker<T>::convert(opt.value())
				+ std::string(" )");
		} else {
			return "{ }";
		}
	}
};
}

#endif /* NEWSBOAT_TEST_HELPERS_STRINGMAKER_OPTIONAL_H_ */

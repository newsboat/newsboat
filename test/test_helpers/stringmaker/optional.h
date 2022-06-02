#ifndef NEWSBOAT_TEST_HELPERS_STRINGMAKER_OPTIONAL_H_
#define NEWSBOAT_TEST_HELPERS_STRINGMAKER_OPTIONAL_H_

#include "3rd-party/optional.hpp"
#include "3rd-party/catch.hpp"

namespace Catch {
template<typename T>
struct StringMaker<nonstd::optional<T>> {
	static std::string convert(const nonstd::optional<T>& opt)
	{
		if (opt.has_value()) {
			return std::string("nonstd::optional( ")
				+ StringMaker<T>::convert(opt.value())
				+ std::string(" )");
		} else {
			return "{ }";
		}
	}
};
}

#endif /* NEWSBOAT_TEST_HELPERS_STRINGMAKER_OPTIONAL_H_ */

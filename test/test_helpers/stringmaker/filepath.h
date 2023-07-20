#ifndef NEWSBOAT_TEST_HELPERS_STRINGMAKER_FILEPATH_H_
#define NEWSBOAT_TEST_HELPERS_STRINGMAKER_FILEPATH_H_

#include "3rd-party/catch.hpp"
// Can't spell this as "filepath.h" because then this file will try to include
// itself.
#include "include/filepath.h"

namespace Catch {
template<>
struct StringMaker<newsboat::Filepath> {
	static std::string convert(const newsboat::Filepath& filepath)
	{
		return std::string("Filepath( \"")
			+ filepath.display()
			+ std::string("\" )");
	}
};
}

#endif /* NEWSBOAT_TEST_HELPERS_STRINGMAKER_FILEPATH_H_ */

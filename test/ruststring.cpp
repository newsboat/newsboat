#include "ruststring.h"

#include "3rd-party/catch.hpp"

using namespace Newsboat;

extern "C" char* rs_get_string(const char* line);

TEST_CASE("RustString constructor and conversion to std::string",
	"[RustString]")
{
	RustString foo(rs_get_string("foo"));
	REQUIRE(std::string(foo) == std::string("foo"));

	RustString bar(nullptr);
	REQUIRE(std::string(bar) == std::string());
}

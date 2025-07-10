#include "matcherexception.h"

#include "3rd-party/catch.hpp"

#include <cstring>

using namespace Newsboat;

extern "C" {
	MatcherErrorFfi rs_get_test_attr_unavail_error();
	MatcherErrorFfi rs_get_test_invalid_regex_error();
}

TEST_CASE("Can be constructed from Rust error returned over FFI",
	"[MatcherException]")
{
	SECTION("Attribute unavailable") {
		const auto e = MatcherException::from_rust_error(
				rs_get_test_attr_unavail_error());
		REQUIRE(e.type() == MatcherException::Type::ATTRIB_UNAVAIL);
		REQUIRE(e.info() == "test_attribute");
		REQUIRE(e.info2().empty());
		REQUIRE_FALSE(strlen(e.what()) == 0);
	}

	SECTION("Invalid regex") {
		const auto e = MatcherException::from_rust_error(
				rs_get_test_invalid_regex_error());
		REQUIRE(e.type() == MatcherException::Type::INVALID_REGEX);
		REQUIRE(e.info() == "?!");
		REQUIRE(e.info2() == "inconceivable happened!");
		REQUIRE_FALSE(strlen(e.what()) == 0);
	}
}

#include "formatstring.h"

#include "3rd-party/catch.hpp"

using namespace newsboat;

TEST_CASE("do_format replaces variables with values", "[fmtstr_formatter]") {
	fmtstr_formatter fmt;

	SECTION("One format variable") {
		fmt.register_fmt('a',"AAA");

		SECTION("Conditional format strings") {
			REQUIRE(fmt.do_format("%?a?%a&no?") == "AAA");
			REQUIRE(fmt.do_format("%?b?%b&no?") == "no");
			REQUIRE(fmt.do_format("%?a?[%-4a]&no?") == "[AAA ]");
		}

		SECTION("Two format variables") {
			fmt.register_fmt('b',"BBB");

			REQUIRE(fmt.do_format("asdf | %a | %?c?%a%b&%b%a? | qwert") == "asdf | AAA | BBBAAA | qwert");
			REQUIRE(fmt.do_format("%?c?asdf?") == "");

			SECTION("Three format variables") {
				fmt.register_fmt('c',"CCC");

				SECTION("Simple cases") {
					REQUIRE(fmt.do_format("") == "");
					// illegal single %
					REQUIRE(fmt.do_format("%") == "");
					REQUIRE(fmt.do_format("%%") == "%");
					REQUIRE(fmt.do_format("%a%b%c") == "AAABBBCCC");
					REQUIRE(fmt.do_format("%%%a%%%b%%%c%%") == "%AAA%BBB%CCC%");
				}

				SECTION("Alignment") {
					REQUIRE(fmt.do_format("%4a") == " AAA");
					REQUIRE(fmt.do_format("%-4a") == "AAA ");

					SECTION("Alignment limits") {
						REQUIRE(fmt.do_format("%2a") == "AA");
						REQUIRE(fmt.do_format("%-2a") == "AA");
					}
				}

				SECTION("Complex format string") {
					REQUIRE(fmt.do_format("<%a> <%5b> | %-5c%%") == "<AAA> <  BBB> | CCC  %");
					REQUIRE(fmt.do_format("asdf | %a | %?c?%a%b&%b%a? | qwert") == "asdf | AAA | AAABBB | qwert");
				}

				SECTION("Format string fillers") {
					REQUIRE(fmt.do_format("%>X", 3) == "XXX");
					REQUIRE(fmt.do_format("%a%> %b", 10) == "AAA    BBB");
					REQUIRE(fmt.do_format("%a%> %b", 0) == "AAA BBB");
				}

				SECTION("Conditional format string") {
					REQUIRE(fmt.do_format("%?c?asdf?") == "asdf");
				}
			}
		}
	}
}

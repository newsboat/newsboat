#include "fmtstrformatter.h"

#include "3rd-party/catch.hpp"

using namespace Newsboat;

TEST_CASE("do_format replaces variables with values", "[FmtStrFormatter]")
{
	FmtStrFormatter fmt;

	SECTION("One format variable") {
		fmt.register_fmt('a', "AAA");

		SECTION("Conditional format strings") {
			REQUIRE(fmt.do_format("%?a?%a&no?") == "AAA");
			REQUIRE(fmt.do_format("%?b?%b&no?") == "no");
			REQUIRE(fmt.do_format("%?a?[%-4a]&no?") == "[AAA ]");
		}

		SECTION("Two format variables") {
			fmt.register_fmt('b', "BBB");

			REQUIRE(fmt.do_format(
					"asdf | %a | %?c?%a%b&%b%a? | qwert") ==
				"asdf | AAA | BBBAAA | qwert");
			REQUIRE(fmt.do_format("%?c?asdf?") == "");

			SECTION("Three format variables") {
				fmt.register_fmt('c', "CCC");

				SECTION("Simple cases") {
					REQUIRE(fmt.do_format("") == "");
					// illegal single %
					REQUIRE(fmt.do_format("%") == "");
					REQUIRE(fmt.do_format("%%") == "%");
					REQUIRE(fmt.do_format("%a%b%c") ==
						"AAABBBCCC");
					REQUIRE(fmt.do_format(
							"%%%a%%%b%%%c%%") ==
						"%AAA%BBB%CCC%");
				}

				SECTION("Alignment") {
					REQUIRE(fmt.do_format("%4a") == " AAA");
					REQUIRE(fmt.do_format("%-4a") ==
						"AAA ");

					SECTION("Alignment limits") {
						REQUIRE(fmt.do_format("%2a") ==
							"AA");
						REQUIRE(fmt.do_format("%-2a") ==
							"AA");
					}
				}

				SECTION("Complex format string") {
					REQUIRE(fmt.do_format("<%a> <%5b> | "
							"%-5c%%") ==
						"<AAA> <  BBB> | CCC  %");
					REQUIRE(fmt.do_format("asdf | %a | "
							"%?c?%a%b&%b%a? "
							"| qwert") ==
						"asdf | AAA | AAABBB | qwert");
				}

				SECTION("Format string fillers") {
					REQUIRE(fmt.do_format("%>X", 3) ==
						"XXX");
					REQUIRE(fmt.do_format("%a%> %b", 10) ==
						"AAA    BBB");
					REQUIRE(fmt.do_format("%a%> %b", 0) ==
						"AAA BBB");
				}

				SECTION("Conditional format string") {
					REQUIRE(fmt.do_format("%?c?asdf?") ==
						"asdf");
				}
			}
		}
	}
}

TEST_CASE("do_format supports multibyte characters", "[FmtStrFormatter]")
{
	FmtStrFormatter fmt;

	SECTION("One format variable") {
		fmt.register_fmt('a', "АБВ");

		SECTION("Conditional format strings") {
			REQUIRE(fmt.do_format("%?a?%a&no?") == "АБВ");
			REQUIRE(fmt.do_format("%?b?%b&no?") == "no");
			REQUIRE(fmt.do_format("%?a?[%-4a]&no?") == "[АБВ ]");
		}

		SECTION("Two format variables") {
			fmt.register_fmt('b', "буква");

			REQUIRE(fmt.do_format(
					"asdf | %a | %?c?%a%b&%b%a? | qwert") ==
				"asdf | АБВ | букваАБВ | qwert");
			REQUIRE(fmt.do_format("%?c?asdf?") == "");

			SECTION("Three format variables") {
				fmt.register_fmt('c', "ещё одна переменная");

				SECTION("Simple cases") {
					REQUIRE(fmt.do_format("") == "");
					// illegal single %
					REQUIRE(fmt.do_format("%") == "");
					REQUIRE(fmt.do_format("%%") == "%");
					REQUIRE(fmt.do_format("%a%b%c") ==
						"АБВбукваещё одна переменная");
					REQUIRE(fmt.do_format(
							"%%%a%%%b%%%c%%") ==
						"%АБВ%буква%ещё одна переменная%");
				}

				SECTION("Alignment") {
					REQUIRE(fmt.do_format("%4a") == " АБВ");
					REQUIRE(fmt.do_format("%-4a") ==
						"АБВ ");

					SECTION("Alignment limits") {
						REQUIRE(fmt.do_format("%2a") ==
							"АБ");
						REQUIRE(fmt.do_format("%-2a") ==
							"АБ");
					}
				}

				SECTION("Complex format string") {
					REQUIRE(fmt.do_format("<%a> <%5b> | "
							"%-5c%%") ==
						"<АБВ> <буква> | ещё о%");
					REQUIRE(fmt.do_format("asdf | %a | "
							"%?c?%a%b&%b%a? "
							"| qwert") ==
						"asdf | АБВ | АБВбуква | qwert");
				}

				SECTION("Format string fillers") {
					REQUIRE(fmt.do_format("%>X", 3) ==
						"XXX");
					REQUIRE(fmt.do_format("%a%> %b", 10) ==
						"АБВ  буква");
					REQUIRE(fmt.do_format("%a%> %b", 0) ==
						"АБВ буква");
				}

				SECTION("Conditional format string") {
					REQUIRE(fmt.do_format("%?c?asdf?") ==
						"asdf");
				}
			}
		}
	}
}

TEST_CASE("do_format() keeps wide characters within specified width",
	"[FmtStrFormatter]")
{
	FmtStrFormatter fmt;

	fmt.register_fmt('a', "ＡＢＣ");
	fmt.register_fmt('b', "def");

	REQUIRE(fmt.do_format("%a %b", 0) == "ＡＢＣ def");
	REQUIRE(fmt.do_format("%a %b", 10) == "ＡＢＣ def");
	REQUIRE(fmt.do_format("%a %b", 9) == "ＡＢＣ de");
	REQUIRE(fmt.do_format("%a %b", 7) == "ＡＢＣ ");
	REQUIRE(fmt.do_format("%a %b", 6) == "ＡＢＣ");
	REQUIRE(fmt.do_format("%a %b", 4) == "ＡＢ");
}

TEST_CASE("do_format() does not include wide character if only 1 colum is left",
	"[FmtStrFormatter]")
{
	FmtStrFormatter fmt;

	fmt.register_fmt('a', "ＡＢＣ");

	REQUIRE(fmt.do_format("%a", 6) == "ＡＢＣ");
	REQUIRE(fmt.do_format("%a", 5) == "ＡＢ");
	REQUIRE(fmt.do_format("%a", 4) == "ＡＢ");

	REQUIRE(fmt.do_format("%-6a", 0) == "ＡＢＣ");
	REQUIRE(fmt.do_format("%-5a", 0) == "ＡＢ ");
	REQUIRE(fmt.do_format("%-4a", 0) == "ＡＢ");

	REQUIRE(fmt.do_format("%6a", 0) == "ＡＢＣ");
	REQUIRE(fmt.do_format("%5a", 0) == " ＡＢ");
	REQUIRE(fmt.do_format("%4a", 0) == "ＡＢ");
}

TEST_CASE("do_format() does not escape less-than signs in regular text",
	"[FmtStrFormatter]")
{
	FmtStrFormatter fmt;

	fmt.register_fmt('a', "AAA");
	fmt.register_fmt('b', "BBB");

	REQUIRE(fmt.do_format("%a <%b>", 0) == "AAA <BBB>");
}

TEST_CASE("do_format() does not escape less-than signs in filling format",
	"[FmtStrFormatter]")
{
	FmtStrFormatter fmt;

	fmt.register_fmt('a', "AAA");
	fmt.register_fmt('b', "BBB");

	REQUIRE(fmt.do_format("%a%>.%b", 10) == "AAA....BBB");
	REQUIRE(fmt.do_format("%a%><%b", 10) == "AAA<<<<BBB");
	REQUIRE(fmt.do_format("%a%>>%b", 10) == "AAA>>>>BBB");
}

TEST_CASE("do_format ignores \"%?\" at the end of the format string (which "
	"looks like a conditional but really isn't",
	"[FmtStrFormatter]")
{
	FmtStrFormatter fmt;
	REQUIRE(fmt.do_format("%?") == "");
}

TEST_CASE("do_format replaces %Nx with the value of \"x\", padded "
	"with spaces on the left to fit N columns",
	"[FmtStrFormatter]")
{
	FmtStrFormatter fmt;

	SECTION("Undefined format char") {
		REQUIRE(fmt.do_format("%4a") == "    ");
		REQUIRE(fmt.do_format("%8a") == "        ");
	}

	SECTION("Defined format char") {
		fmt.register_fmt('a', "hello");
		fmt.register_fmt('r', "привет");

		REQUIRE(fmt.do_format("%8a") == "   hello");
		REQUIRE(fmt.do_format("%5a") == "hello");
		REQUIRE(fmt.do_format("%8r") == "  привет");
		REQUIRE(fmt.do_format("%6r") == "привет");

		{
			INFO("If the value is bigger than the padding, only first N "
				"characters are used");
			REQUIRE(fmt.do_format("%4a") == "hell");
			REQUIRE(fmt.do_format("%2a") == "he");
			REQUIRE(fmt.do_format("%4r") == "прив");
			REQUIRE(fmt.do_format("%3r") == "при");
		}
	}
}

TEST_CASE("do_format replaces %-Nx with the value of \"x\", padded "
	"with spaces on the right to fit N columns",
	"[FmtStrFormatter]")
{
	FmtStrFormatter fmt;

	SECTION("Undefined format char") {
		REQUIRE(fmt.do_format("%-4a") == "    ");
		REQUIRE(fmt.do_format("%-8a") == "        ");
	}

	SECTION("Defined format char") {
		fmt.register_fmt('a', "hello");
		fmt.register_fmt('r', "привет");

		REQUIRE(fmt.do_format("%-8a") == "hello   ");
		REQUIRE(fmt.do_format("%-5a") == "hello");
		REQUIRE(fmt.do_format("%-8r") == "привет  ");
		REQUIRE(fmt.do_format("%-6r") == "привет");

		{
			INFO("If the value is bigger than the padding, only first N "
				"characters are used");
			REQUIRE(fmt.do_format("%-4a") == "hell");
			REQUIRE(fmt.do_format("%-2a") == "he");
			REQUIRE(fmt.do_format("%-4r") == "прив");
			REQUIRE(fmt.do_format("%-3r") == "при");
		}
	}
}

TEST_CASE("%>[char] pads consecutive text to the right, using [char] for "
	"the padding",
	"[FmtStrFormatter]")
{
	FmtStrFormatter fmt;

	fmt.register_fmt('x', "example string");
	fmt.register_fmt('y', "пример строки");

	SECTION("The total length of the string is specified in the argument to do_format") {
		{
			INFO("Default (zero) is \"as long as needed to fit all the values\"");
			REQUIRE(fmt.do_format("%x%> %y") == "example string пример строки");
			REQUIRE(fmt.do_format("%x%> %y",
					0) == "example string пример строки");
		}

		REQUIRE(fmt.do_format("%x%> %y",
				30) == "example string   пример строки");
		REQUIRE(fmt.do_format("%x%> %y", 45)
			== "example string                  пример строки");
	}

	SECTION("[char] is used for padding") {
		REQUIRE(fmt.do_format("%x%>m%y") == "example stringmпример строки");
		REQUIRE(fmt.do_format("%x%>f%y",
				0) == "example stringfпример строки");
		REQUIRE(fmt.do_format("%x%>k%y",
				30) == "example stringkkkпример строки");
		REQUIRE(fmt.do_format("%x%>i%y", 45)
			== "example stringiiiiiiiiiiiiiiiiiiпример строки");
	}

	SECTION("If multiple %>[char] specifiers are used, the first one has a "
		"maximal width and the others have the width of one column") {
		fmt.register_fmt('s', "_short_");

		const auto format = std::string("%s%>a%s%>b%s");
		REQUIRE(fmt.do_format(format) == "_short_a_short_b_short_");
		REQUIRE(fmt.do_format(format, 30) == "_short_aaaaaaaa_short_b_short_");
	}
}

TEST_CASE("%?[char]?[then-format]&[else-format]? is replaced by "
	"\"then-format\" if \"[char]\" has non-empty value, otherwise it's "
	"replaced by \"else-format\"",
	"[FmtStrFormatter]")
{
	FmtStrFormatter fmt;

	fmt.register_fmt('t', "this is a non-empty string");
	fmt.register_fmt('m', "");

	SECTION("Standard case") {
		REQUIRE(fmt.do_format("%?t?non-empty&empty?") == "non-empty");
		REQUIRE(fmt.do_format("%?m?non-empty&empty?") == "empty");
		REQUIRE(fmt.do_format("%?t?непустое&пустое?") ==
			"непустое");
		REQUIRE(fmt.do_format("%?m?непустое&пустое?") == "пустое");
	}

	SECTION("Else-format is optional") {
		REQUIRE(fmt.do_format("%?t?непустое?") == "непустое");
		REQUIRE(fmt.do_format("%?m?непустое?") == "");
	}

	SECTION("Whitespace on its own is handled same as empty") {
		fmt.register_fmt('a', " \t ");
		fmt.register_fmt('b', "  some whitespace  ");

		REQUIRE(fmt.do_format("%?a?non-empty&empty?") == "empty");
		REQUIRE(fmt.do_format("%?b?non-empty&empty?") == "non-empty");
	}
}

TEST_CASE("do_format replaces \"%%\" with a percent sign",
	"[FmtStrFormatter]")
{
	FmtStrFormatter fmt;

	REQUIRE(fmt.do_format("%%") == "%");
}

TEST_CASE("Ampersand is treated literally outside of conditionals",
	"[FmtStrFormatter]")
{
	FmtStrFormatter fmt;

	fmt.register_fmt('a', "A");
	fmt.register_fmt('b', "B");

	REQUIRE(fmt.do_format("%a & %b were sitting on a pipe")
		== "A & B were sitting on a pipe");
}

TEST_CASE("Question mark is treated literally outside of conditionals",
	"[FmtStrFormatter]")
{
	FmtStrFormatter fmt;

	fmt.register_fmt('x', "What's the ultimate answer");
	fmt.register_fmt('y', "42");

	REQUIRE(fmt.do_format("%x? %y") == "What's the ultimate answer? 42");
}

TEST_CASE("Center Text formatting",
	"[FmtStrFormatter]")
{
	FmtStrFormatter fmt;

	fmt.register_fmt('T', "whatever");

	REQUIRE(fmt.do_format("%=20T", 0) ==  "      whatever      ");
	REQUIRE(fmt.do_format("%=19T", 0) == "     whatever      ");
	REQUIRE(fmt.do_format("%=3T", 0) == "wha");
	REQUIRE(fmt.do_format("%=0T", 20) == "      whatever      ");
}

#include "utf8string.h"

#include <locale.h>
#include <string>

#include "3rd-party/catch.hpp"
#include "3rd-party/optional.hpp"
#include "test-helpers/envvar.h"

using namespace newsboat;

TEST_CASE("Utf8String can be constructed from valid UTF-8", "[Utf8String]")
{
	const auto str1 = Utf8String::from_utf8("");
	REQUIRE(str1.to_utf8() == "");

	// Russian "Привет", "hello"
	const auto str2 =
		Utf8String::from_utf8("\xd0\x9f\xd1\x80\xd0\xb8\xd0\xb2\xd0\xb5\xd1\x82");
	REQUIRE(str2.to_utf8() == "Привет");
}

TEST_CASE("Utf8String can be implicitly constructed from a string literal", "[Utf8String]")
{
	const auto check = [](Utf8String actual, std::string expected) {
		REQUIRE(actual.to_utf8() == expected);
	};

	check("", "");
	// Russian "Привет", "hello"
	check("\xd0\x9f\xd1\x80\xd0\xb8\xd0\xb2\xd0\xb5\xd1\x82", "Привет");
}

SCENARIO("Utf8String can be constructed from strings in locale locale charset",
	"[Utf8String]")
{
	TestHelpers::LcCtypeEnvVar lc_ctype;
	const auto try_set_locale = [&lc_ctype](std::string new_locale) -> bool {
		if (::setlocale(LC_CTYPE, new_locale.c_str()) == nullptr)
		{
			WARN("Couldn't set locale " + new_locale + "; test skipped.");
			return false;
		}
		lc_ctype.set(new_locale);
		return true;
	};

	GIVEN("UTF-8 locale") {
		if (!try_set_locale("en_US.UTF-8")) {
			return;
		}

		WHEN("Utf8String is constructed from an empty string") {
			const auto str = Utf8String::from_locale_charset("");

			THEN("the result is empty") {
				REQUIRE(str.to_utf8() == "");
			}

			THEN("the result in locale encoding is empty") {
				REQUIRE(str.to_locale_charset() == "");
			}
		}

		WHEN("Utf8String is constructed from a string \"проверка\"") {
			// Russian "проверка", "check"
			const std::string
			input("\xd0\xbf\xd1\x80\xd0\xbe\xd0\xb2\xd0\xb5\xd1\x80\xd0\xba\xd0\xb0");
			const auto str = Utf8String::from_locale_charset(input);

			THEN("the result is that string in UTF-8") {
				REQUIRE(str.to_utf8() == "проверка");
			}

			THEN("the result in locale encoding is the same as input") {
				REQUIRE(str.to_locale_charset() == input);
			}
		}
	}

	GIVEN("KOI8-R locale") {
		if (!try_set_locale("ru_RU.KOI8-R")) {
			return;
		}

		WHEN("Utf8String is constructed from an empty string") {
			const auto str = Utf8String::from_locale_charset("");

			THEN("the result is empty") {
				REQUIRE(str.to_utf8() == "");
			}

			THEN("the result in locale encoding is empty") {
				REQUIRE(str.to_locale_charset() == "");
			}
		}

		WHEN("Utf8String is constructed from a string \"строка\"") {
			// Russian "строка", "string"
			const std::string input("\xd3\xd4\xd2\xcf\xcb\xc1");
			const auto str = Utf8String::from_locale_charset(input);

			THEN("the result is that string in UTF-8") {
				REQUIRE(str.to_utf8() == "строка");
			}

			THEN("the result in locale encoding is the same as input") {
				REQUIRE(str.to_locale_charset() == input);
			}
		}
	}

	GIVEN("CP1251 locale") {
		if (!try_set_locale("ru_RU.CP1251")) {
			return;
		}

		WHEN("Utf8String is constructed from an empty string") {
			const auto str = Utf8String::from_locale_charset("");

			THEN("the result is empty") {
				REQUIRE(str.to_utf8() == "");
			}

			THEN("the result in locale encoding is empty") {
				REQUIRE(str.to_locale_charset() == "");
			}
		}

		WHEN("Utf8String is constructed from string \"окна\"") {
			// Russian "окна", "windows"
			const std::string input("\xee\xea\xed\xe0");
			const auto str = Utf8String::from_locale_charset(input);

			THEN("the result is that string in UTF-8") {
				REQUIRE(str.to_utf8() == "окна");
			}

			THEN("the result in locale encoding is the same as input") {
				REQUIRE(str.to_locale_charset() == input);
			}
		}
	}
}

TEST_CASE("Utf8String::from_locale_charset() replaces invalid codepoints of locale charset with question marks",
	"[Utf8String]")
{
	TestHelpers::LcCtypeEnvVar lc_ctype;
	const auto try_set_locale = [&lc_ctype](std::string new_locale) -> bool {
		if (::setlocale(LC_CTYPE, new_locale.c_str()) == nullptr)
		{
			WARN("Couldn't set locale " + new_locale + "; test skipped.");
			return false;
		}
		lc_ctype.set(new_locale);
		return true;
	};

	SECTION("UTF-8 locale") {
		if (!try_set_locale("en_US.UTF-8")) {
			return;
		}

		// Russian "мотоцикл", "motorbike", with a few bytes missing in the middle
		const std::string input("\xd0\xbc\xd0\xbe\xd1\x82\xbe\xd1\xd0\xb8\xd0\xba\xd0\xbb");
		const auto str = Utf8String::from_locale_charset(input);

		REQUIRE(str.to_utf8() == "мот??икл");
	}

	// There is no tests for KOI8-R and CP1251 because there is no invalid codepoints there.
}

TEST_CASE("Utf8String::to_locale_charset() replaces invalid characters with question marks",
	"[Utf8String]")
{
	TestHelpers::LcCtypeEnvVar lc_ctype;
	const auto try_set_locale = [&lc_ctype](std::string new_locale) -> bool {
		if (::setlocale(LC_CTYPE, new_locale.c_str()) == nullptr)
		{
			WARN("Couldn't set locale " + new_locale + "; test skipped.");
			return false;
		}
		lc_ctype.set(new_locale);
		return true;
	};

	GIVEN("UTF-8 locale") {
		if (!try_set_locale("en_US.UTF-8")) {
			return;
		}

		WHEN("Utf8String is constructed from a string \"повтор\"") {
			// Russian "повтор", "repetition"
			const std::string input("\xd0\xbf\xd0\xbe\xd0\xb2\xd1\x82\xd0\xbe\xd1\x80");
			const auto str = Utf8String::from_utf8(input);

			THEN("the result represents the string faithfully") {
				REQUIRE(str.to_utf8() == input);
			}

			THEN("the result in locale encoding is the same as input") {
				REQUIRE(str.to_locale_charset() == input);
			}
		}
	}

	GIVEN("KOI8-R locale") {
		if (!try_set_locale("ru_RU.KOI8-R")) {
			return;
		}

		WHEN("Utf8String is constructed from a string \"привіт\"") {
			// Ukrainian "привіт", "hello" -- KOI8-R can't represent "і'
			const std::string input("\xd0\xbf\xd1\x80\xd0\xb8\xd0\xb2\xd1\x96\xd1\x82");
			const auto str = Utf8String::from_utf8(input);

			THEN("the result represents the string faithfully") {
				REQUIRE(str.to_utf8() == input);
			}

			THEN("the result in locale encoding has a question mark where \"і\" should be") {
				REQUIRE(str.to_locale_charset() == "\xd0\xd2\xc9\xd7\x3f\xd4");
			}
		}
	}

	GIVEN("CP1251 locale") {
		if (!try_set_locale("ru_RU.CP1251")) {
			return;
		}

		WHEN("Utf8String is constructed from string \"Приветствуем, 日本!\"") {
			// "Приветствуем, 日本!", "Greetings, Japan!" (a mix of Russian and
			// Japanese). CP1251 can't represent Japanese characters, but can
			// represent Russian and the exclamation mark
			const std::string
			input("\xd0\x9f\xd1\x80\xd0\xb8\xd0\xb2\xd0\xb5\xd1\x82\xd1\x81\xd1\x82\xd0\xb2\xd1\x83\xd0\xb5\xd0\xbc\x2c\x20\xe6\x97\xa5\xe6\x9c\xac\x21");
			const auto str = Utf8String::from_utf8(input);

			THEN("the result represents the string faithfully") {
				REQUIRE(str.to_utf8() == input);
			}

			THEN("the result in locale encoding has question marks instead of Japanese characters") {
				REQUIRE(str.to_locale_charset() ==
					"\xcf\xf0\xe8\xe2\xe5\xf2\xf1\xf2\xe2\xf3\xe5\xec\x2c\x20\x3f\x3f\x21");
			}
		}
	}
}

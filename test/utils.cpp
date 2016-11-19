#include "catch.hpp"

#include <utils.h>

using namespace newsbeuter;

TEST_CASE("Tokenizers behave correctly", "[utils]") {
	std::vector<std::string> tokens;

	SECTION("utils::tokenize()") {
		tokens = utils::tokenize("as df qqq");
		// tokenize results in 3 tokens
		REQUIRE(tokens.size() == 3);
		// tokenize of as df qqq
		REQUIRE(tokens[0] == "as");
		REQUIRE(tokens[1] == "df");
		REQUIRE(tokens[2] == "qqq");

		tokens = utils::tokenize(" aa ");
		// tokenize of ' aa ' resulted in one token
		REQUIRE(tokens.size() == 1);
		// first token is aa
		REQUIRE(tokens[0] == "aa");

		tokens = utils::tokenize("	");
		// tokenize of tab resulted in 0 tokens
		REQUIRE(tokens.size() == 0);

		tokens = utils::tokenize("");
		// tokenize of empty string resulted in 0 tokens
		REQUIRE(tokens.size() == 0);
	}

	SECTION("utils::tokenize_spaced()") {
		tokens = utils::tokenize_spaced("a b");
		// tokenize_spaced of a b resulted in 3 tokens
		REQUIRE(tokens.size() == 3);
		// middle token is space
		REQUIRE(tokens[1] == " ");

		tokens = utils::tokenize_spaced(" a\t b ");
		// tokenize_spaced resulted in 5 tokens
		REQUIRE(tokens.size() == 5);
		// first token is space
		REQUIRE(tokens[0] == " ");
		// second token is a
		REQUIRE(tokens[1] == "a");
		// third token is a tab followed by a space
		REQUIRE(tokens[2] == "\t ");
		// fourth token is b
		REQUIRE(tokens[3] == "b");
		// fifth token is space
		REQUIRE(tokens[4] == " ");
	}

	SECTION("utils::tokenize_quoted()") {
		SECTION("Simple case") {
			tokens = utils::tokenize_quoted("asdf \"foobar bla\" \"foo\\r\\n\\tbar\"");
			// tokenize_quoted resulted in 3 tokens
			REQUIRE(tokens.size() == 3);
			// first token is asdf
			REQUIRE(tokens[0] == "asdf");
			// second token is foobar bla
			REQUIRE(tokens[1] == "foobar bla");
			// third token contains \\r\\n\\t
			REQUIRE(tokens[2] == "foo\r\n\tbar");

			tokens = utils::tokenize_quoted("  \"foo \\\\xxx\"\t\r \" \"");
			// tokenize_quoted resulted in 2 tokens
			REQUIRE(tokens.size() == 2);
			// first token contains "foo \\xxx"
			REQUIRE(tokens[0] == "foo \\xxx");
			// second token is space
			REQUIRE(tokens[1] == " ");
		}

		SECTION("Unbalanced quotes") {
			tokens = utils::tokenize_quoted("\"\\\\");
			// tokenize_quoted with unbalanced quotes resulted in 1 token
			REQUIRE(tokens.size() == 1);
			// first token is backslash
			REQUIRE(tokens[0] == "\\");
		}

		SECTION("Several sequential backslashes") {
			// tokenize_quoted with several \\ sequences directly appended
			tokens = utils::tokenize_quoted("\"\\\\\\\\");
			// tokenize_quoted escape test 1 resulted in 1 token
			REQUIRE(tokens.size() == 1);
			// tokenize_quoted escape test 1 token is two backslashes
			REQUIRE(tokens[0] == "\\\\");

			tokens = utils::tokenize_quoted("\"\\\\\\\\\\\\");
			// tokenize_quoted escape test 2 resulted in 1 token
			REQUIRE(tokens.size() == 1);
			// tokenize_quoted escape test 2 token is three backslashes
			REQUIRE(tokens[0] == "\\\\\\");

			tokens = utils::tokenize_quoted("\"\\\\\\\\\"");
			// tokenize_quoted escape test 3 resulted in 1 token
			REQUIRE(tokens.size() == 1);
			// tokenize_quoted escape test 3 token is two backslashes
			REQUIRE(tokens[0] == "\\\\");

			tokens = utils::tokenize_quoted("\"\\\\\\\\\\\\\"");
			// tokenize_quoted escape test 4 resulted in 1 token
			REQUIRE(tokens.size() == 1);
			// tokenize_quoted escape test 4 token is three backslashes
			REQUIRE(tokens[0] == "\\\\\\");
		}

		SECTION("Escaped backticks should NOT be touched") {
			tokens = utils::tokenize_quoted("asdf \"\\`foobar `bla`\\`\"");

			REQUIRE(tokens.size() == 2);
			REQUIRE(tokens[0] == "asdf");
			REQUIRE(tokens[1] == "\\`foobar `bla`\\`");
		}
	}
}

TEST_CASE("consolidate_whitespace replaces multiple consecutine"
          "whitespace with a single space", "[utils]") {
	REQUIRE(utils::consolidate_whitespace("LoremIpsum") == "LoremIpsum");
	REQUIRE(utils::consolidate_whitespace("Lorem Ipsum") == "Lorem Ipsum");
	REQUIRE(utils::consolidate_whitespace("    Lorem \t\tIpsum \t ") == " Lorem Ipsum ");
	REQUIRE(utils::consolidate_whitespace("    Lorem \r\n\r\n\tIpsum") == " Lorem Ipsum");

	REQUIRE(utils::consolidate_whitespace("") == "");

	REQUIRE(utils::consolidate_whitespace("  Lorem|||Ipsum||", "|") == "  Lorem Ipsum ");
}

TEST_CASE("String conversions behave correctly", "[utils]") {
	SECTION("Conversion from wstring to string") {
		std::string s1 = utils::wstr2str(L"This is a simple string. Let's have a look at the outcome...");
		REQUIRE(s1 == "This is a simple string. Let's have a look at the outcome...");

		std::string s2 = utils::wstr2str(L"");
		REQUIRE(s2 == "");
	}

	SECTION("Conversion from string to wstring") {
		std::wstring w1 = utils::str2wstr("And that's another simple string.");
		REQUIRE(w1 == L"And that's another simple string.");

		std::wstring w2 = utils::str2wstr("");
		REQUIRE(w2 == L"");
	}
}

TEST_CASE("get_command_output()", "[utils]") {
	REQUIRE(utils::get_command_output("ls /dev/null") == "/dev/null\n");
}

TEST_CASE("run_program()", "[utils]") {
	char * argv[4];
	char cat[] = "cat";
	argv[0] = cat;
	argv[1] = nullptr;
	REQUIRE(utils::run_program(argv, "this is a multine-line\ntest string") == "this is a multine-line\ntest string");

	char echo[] = "echo";
	char dashn[] = "-n";
	char helloworld[] = "hello world";
	argv[0] = echo;
	argv[1] = dashn;
	argv[2] = helloworld;
	argv[3] = nullptr;
	REQUIRE(utils::run_program(argv, "") == "hello world");
}

TEST_CASE("replace_all()", "[utils]") {
	REQUIRE(utils::replace_all("aaa", "a", "b") == "bbb");
	REQUIRE(utils::replace_all("aaa", "aa", "ba") == "baa");
	REQUIRE(utils::replace_all("aaaaaa", "aa", "ba") == "bababa");
	REQUIRE(utils::replace_all("", "a", "b") == "");
	REQUIRE(utils::replace_all("aaaa", "b", "c") == "aaaa");
	REQUIRE(utils::replace_all("this is a normal test text", " t", " T") == "this is a normal Test Text");
	REQUIRE(utils::replace_all("o o o", "o", "<o>") == "<o> <o> <o>");
}

TEST_CASE("to_string()", "[utils]") {
	REQUIRE(utils::to_string<int>(0) == "0");
	REQUIRE(utils::to_string<int>(100) == "100");
	REQUIRE(utils::to_string<unsigned int>(65536) == "65536");
	REQUIRE(utils::to_string<unsigned int>(65537) == "65537");
}

TEST_CASE("partition_index()", "[utils]") {
	std::vector<std::pair<unsigned int, unsigned int>> partitions;

	SECTION("[0, 9] into 2") {
		partitions = utils::partition_indexes(0, 9, 2);
		// partitioning of [0,9] in 2 parts produced 2 parts
		REQUIRE(partitions.size() == 2);
		// first partition start is 0
		REQUIRE(partitions[0].first == 0);
		// first partition end is 4
		REQUIRE(partitions[0].second == 4);
		// second partition start is 5
		REQUIRE(partitions[1].first == 5);
		// second partition end is 9
		REQUIRE(partitions[1].second == 9);
	}

	SECTION("[0, 10] into 3") {
		partitions = utils::partition_indexes(0, 10, 3);
		// partitioning of [0,10] in 3 parts produced 3 parts
		REQUIRE(partitions.size() == 3);
		// first partition start is 0
		REQUIRE(partitions[0].first == 0);
		// first partition end is 2
		REQUIRE(partitions[0].second == 2);
		// second partition start is 3
		REQUIRE(partitions[1].first == 3);
		// second partition end is 5
		REQUIRE(partitions[1].second == 5);
		// third partition start is 6
		REQUIRE(partitions[2].first == 6);
		// third partition end is 10
		REQUIRE(partitions[2].second == 10);
	}

	SECTION("[0, 11] into 3") {
		partitions = utils::partition_indexes(0, 11, 3);
		// partitioning of [0,11] in 3 parts produced 3 parts
		REQUIRE(partitions.size() == 3);
		// first partition start is 0
		REQUIRE(partitions[0].first == 0);
		// first partition end is 3
		REQUIRE(partitions[0].second == 3);
		// second partition start is 4
		REQUIRE(partitions[1].first == 4);
		// second partition end is 7
		REQUIRE(partitions[1].second == 7);
		// third partition start is 8
		REQUIRE(partitions[2].first == 8);
		// third partition end is 11
		REQUIRE(partitions[2].second == 11);
	}

	SECTION("[0, 199] into 200") {
		partitions = utils::partition_indexes(0, 199, 200);
		// partitioning of [0,199] in 200 parts produced 200 parts
		REQUIRE(partitions.size() == 200);
	}

	SECTION("[0, 103] into 1") {
		partitions = utils::partition_indexes(0, 103, 1);
		// partitioning of [0,103] in 1 partition produced 1 partition
		REQUIRE(partitions.size() == 1);
		// first partition start is 0
		REQUIRE(partitions[0].first == 0);
		// first partition end is 103
		REQUIRE(partitions[0].second == 103);
	}
}

TEST_CASE("censor_url()", "[utils]") {
	// censor empty string
	REQUIRE(utils::censor_url("") == "");
	// censor foobar
	REQUIRE(utils::censor_url("foobar") == "foobar");
	// censor foobar: url with no authinfo
	REQUIRE(utils::censor_url("foobar://xyz/") == "foobar://xyz/");

	// censor http url with no authinfo
	REQUIRE(utils::censor_url("http://newsbeuter.org/") == "http://newsbeuter.org/");
	// censor https url with no authinfo
	REQUIRE(utils::censor_url("https://newsbeuter.org/") == "https://newsbeuter.org/");

	// censor http url with empty authinfo
	REQUIRE(utils::censor_url("http://@newsbeuter.org/") == "http://*:*@newsbeuter.org/");
	// censor https url with empty authinfo
	REQUIRE(utils::censor_url("https://@newsbeuter.org/") == "https://*:*@newsbeuter.org/");

	// censor http url with authinfo
	REQUIRE(utils::censor_url("http://foo:bar@newsbeuter.org/") == "http://*:*@newsbeuter.org/");
	// censor https url with authinfo
	REQUIRE(utils::censor_url("https://foo:bar@newsbeuter.org/") == "https://*:*@newsbeuter.org/");

	// censor http url with username-only authinfo
	REQUIRE(utils::censor_url("http://aschas@newsbeuter.org/") == "http://*:*@newsbeuter.org/");
	// censor https url with username-only authinfo
	REQUIRE(utils::censor_url("https://aschas@newsbeuter.org/") == "https://*:*@newsbeuter.org/");

	// censor xxx url with username-only authinfo
	REQUIRE(utils::censor_url("xxx://aschas@newsbeuter.org/") == "xxx://*:*@newsbeuter.org/");

	// censor http url with no authinfo and no trailing slash
	REQUIRE(utils::censor_url("http://foobar") == "http://foobar");
	// censor https url with no authinfo and no trailing slash
	REQUIRE(utils::censor_url("https://foobar") == "https://foobar");

	// censor http url with username-only authinfo and no trailing slash
	REQUIRE(utils::censor_url("http://aschas@host") == "http://*:*@host");
	// censor http url with username-only authinfo and no trailing slash
	REQUIRE(utils::censor_url("https://aschas@host") == "https://*:*@host");

	// censor query feed
	REQUIRE(utils::censor_url("query:name:age between 1:10") == "query:name:age between 1:10");
}

TEST_CASE("absolute_url()", "[utils]") {
	REQUIRE(utils::absolute_url("http://foobar/hello/crook/", "bar.html")
			== "http://foobar/hello/crook/bar.html");
	REQUIRE(utils::absolute_url("https://foobar/foo/", "/bar.html")
			== "https://foobar/bar.html");
	REQUIRE(utils::absolute_url("https://foobar/foo/", "http://quux/bar.html")
			== "http://quux/bar.html");
	REQUIRE(utils::absolute_url("http://foobar", "bla.html")
			== "http://foobar/bla.html");
	REQUIRE(utils::absolute_url("http://test:test@foobar:33", "bla2.html")
			== "http://test:test@foobar:33/bla2.html");
}

TEST_CASE("quote()", "[utils]") {
	REQUIRE(utils::quote("") == "\"\"");
	REQUIRE(utils::quote("hello world") == "\"hello world\"");
	REQUIRE(utils::quote("\"hello world\"") == "\"\\\"hello world\\\"\"");
}

TEST_CASE("to_u()", "[utils]") {
	REQUIRE(utils::to_u("0") == 0);
	REQUIRE(utils::to_u("23") == 23);
	REQUIRE(utils::to_u("") == 0);
}

TEST_CASE("strwidth()", "[utils]") {
	// empty string is 0 colums wide
	REQUIRE(utils::strwidth("") == 0);

	// xx is 2 columns wide
	REQUIRE(utils::strwidth("xx") == 2);

	// character U+F91F is 2 columns wide
	REQUIRE(utils::strwidth(utils::wstr2str(L"\uF91F")) == 2);
}

TEST_CASE("join()", "[utils]") {
	std::vector<std::string> str;
	REQUIRE(utils::join(str, "") == "");
	REQUIRE(utils::join(str, "-") == "");

	SECTION("Join of one element") {
		str.push_back("foobar");
		REQUIRE(utils::join(str, "") == "foobar");
		REQUIRE(utils::join(str, "-") == "foobar");

		SECTION("Join of two elements") {
			str.push_back("quux");
			REQUIRE(utils::join(str, "") == "foobarquux");
			REQUIRE(utils::join(str, "-") == "foobar-quux");
		}
	}
}

TEST_CASE("trim()", "[utils]") {
	std::string str = "  xxx\r\n";
	utils::trim(str);
	REQUIRE(str == "xxx");

	str = "\n\n abc  foobar\n";
	utils::trim(str);
	REQUIRE(str == "abc  foobar");

	str = "";
	utils::trim(str);
	REQUIRE(str == "");

	str = "     \n";
	utils::trim(str);
	REQUIRE(str == "");
}

TEST_CASE("trim_end()", "[utils]") {
	std::string str = "quux\n";
	utils::trim_end(str);
	REQUIRE(str == "quux");
}

TEST_CASE("utils::make_title extracts possible title from URL") {
	SECTION("Uses last part of URL as title") {
		auto input = "http://example.com/Item";
		REQUIRE(utils::make_title(input) == "Item");
	}

	SECTION("Replaces dashes and underscores with spaces") {
		std::string input;

		SECTION("Dashes") {
			input = "http://example.com/This-is-the-title";
		}

		SECTION("Underscores") {
			input = "http://example.com/This_is_the_title";
		}

		SECTION("Mix of dashes and underscores") {
			input = "http://example.com/This_is-the_title";
		}

		REQUIRE(utils::make_title(input) == "This is the title");
	}

	SECTION("Capitalizes first letter of extracted title") {
		auto input = "http://example.com/this-is-the-title";
		REQUIRE(utils::make_title(input) == "This is the title");
	}

	SECTION("Only cares about last component of the URL") {
		auto input = "http://example.com/items/misc/this-is-the-title";
		REQUIRE(utils::make_title(input) == "This is the title");
	}

	SECTION("Strips out trailing slashes") {
		std::string input;

		SECTION("One slash") {
			input = "http://example.com/item/";
		}

		SECTION("Numerous slashes") {
			input = "http://example.com/item/////////////";
		}

		REQUIRE(utils::make_title(input) == "Item");
	}

	SECTION("Doesn't mind invalid URL scheme") {
		auto input = "blahscheme://example.com/this-is-the-title";
		REQUIRE(utils::make_title(input) == "This is the title");
	}

	SECTION("Strips out URL query parameters") {
		SECTION("Single parameter") {
			auto input = "http://example.com/story/aug/title-with-dashes?a=b";
			REQUIRE(utils::make_title(input) == "Title with dashes");
		}

		SECTION("Multiple parameters") {
			auto input = "http://example.com/title-with-dashes?a=b&x=y&utf8=✓";
			REQUIRE(utils::make_title(input) == "Title with dashes");
		}
	}
}

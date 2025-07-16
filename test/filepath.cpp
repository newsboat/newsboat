#include "3rd-party/catch.hpp"

#include "filepath.h"

using namespace newsboat;

TEST_CASE("Can be constructed from a string and converted back into a string",
	"[Filepath]")
{
	const auto example = Filepath::from_locale_string("/etc/hosts");
	REQUIRE(example.to_locale_string() == "/etc/hosts");
}

TEST_CASE("Can be constructed from non-Unicode data", "[Filepath]")
{
	// See Table 3-7 "Well-Formed UTF-8 Byte Sequences"
	// https://www.unicode.org/versions/Unicode15.0.0/ch03.pdf#G27506
	const std::string input("/invalid: \x81\x82 but it's fine");
	const auto filepath = Filepath::from_locale_string(input);
	REQUIRE(filepath.to_locale_string() == input);
}

TEST_CASE("Can be displayed even if it contains non-Unicode characters", "[Filepath]")
{
	// See Table 3-7 "Well-Formed UTF-8 Byte Sequences"
	// https://www.unicode.org/versions/Unicode15.0.0/ch03.pdf#G27506
	const std::string input("/invalid: \x80 but it's fine");
	const auto filepath = Filepath::from_locale_string(input);
	// 0xef 0xbf 0xbd is UTF-8 encoding of U+FFFD REPLACEMENT CHARACTER
	REQUIRE(filepath.display() == "/invalid: \xEF\xBF\xBD but it's fine");
}

TEST_CASE("push() adds a new component to the path", "[Filepath]")
{
	auto dir = Filepath::from_locale_string("/tmp");

	dir.push(Filepath::from_locale_string("newsboat"));
	REQUIRE(dir == Filepath::from_locale_string("/tmp/newsboat"));

	dir.push(Filepath::from_locale_string(".local/share/cache/cache.db"));
	REQUIRE(dir == Filepath::from_locale_string("/tmp/newsboat/.local/share/cache/cache.db"));
}

TEST_CASE("Can be extended with join()", "[Filepath]")
{
	const auto tmp = Filepath::from_locale_string("/tmp");

	const auto subdir = tmp.join("newsboat").join("tests");
	REQUIRE(subdir == Filepath::from_locale_string("/tmp/newsboat/tests"));
}

TEST_CASE("Can be copied", "[Filepath]")
{
	auto original = Filepath::from_locale_string("/etc/hosts");

	const auto check = [&original](const Filepath& copy) {
		REQUIRE(original == copy);
		REQUIRE(original.display() == "/etc/hosts");

		// Demonstrate that changing the original object doesn't modify the copy
		original.push(Filepath::from_locale_string(" a bit more"));
		REQUIRE(original.display() == "/etc/hosts/ a bit more");
		REQUIRE(copy.display() == "/etc/hosts");
	};

	SECTION("with copy constructor") {
		const Filepath copy(original);
		check(copy);
	}

	SECTION("with copy assignment operator") {
		const Filepath copy = original;
		check(copy);
	}
}

TEST_CASE("Can't set extension for an empty path", "[Filepath]")
{
	Filepath path;
	REQUIRE_FALSE(path.set_extension("exe"));
}

TEST_CASE("Can set extension for non-empty path", "[Filepath]")
{
	Filepath path("file");

	SECTION("extension is UTF-8") {
		REQUIRE(path.set_extension("exe"));
		REQUIRE(path == "file.exe");
	}

	SECTION("extension is not a valid UTF-8 string") {
		REQUIRE(path.set_extension("\x80"));
		REQUIRE(path == Filepath::from_locale_string("file.\x80"));
	}
}

TEST_CASE("Can check if path is absolute", "[Filepath]")
{
	Filepath path;
	SECTION("empty path is not absolute") {
		REQUIRE_FALSE(path.is_absolute());
	}

	SECTION("path that starts with a slash is absolute") {
		path.push("/etc");
		REQUIRE(path.display() == "/etc");
		REQUIRE(path.is_absolute());

		path.push("ca-certificates");
		REQUIRE(path.display() == "/etc/ca-certificates");
		REQUIRE(path.is_absolute());
	}

	SECTION("path that doesn't start with a slash is not absolute") {
		path.push("vmlinuz");
		REQUIRE(path.display() == "vmlinuz");
		REQUIRE_FALSE(path.is_absolute());

		path.push("undefined");
		REQUIRE(path.display() == "vmlinuz/undefined");
		REQUIRE_FALSE(path.is_absolute());
	}
}

TEST_CASE("Can check if path starts with a given base path", "[Filepath]")
{
	SECTION("Empty path") {
		Filepath path;

		SECTION("Base path is empty") {
			REQUIRE(path.starts_with(Filepath{}));
		}

		SECTION("Base path is not empty") {
			REQUIRE_FALSE(path.starts_with(Filepath::from_locale_string("/etc")));
		}
	}

	SECTION("Non-empty path that doesn't start with the base") {
		const auto path = Filepath::from_locale_string("/etcetera");
		const auto base = Filepath::from_locale_string("/etc");
		REQUIRE_FALSE(path.starts_with(base));
	}

	SECTION("Non-empty path that starts with the base") {
		const auto path = Filepath::from_locale_string("/usr/local/bin/newsboat");
		const auto base = Filepath::from_locale_string("/usr/local");
		REQUIRE(path.starts_with(base));
	}

	SECTION("Base is not a valid UTF-8 string") {
		const auto path = Filepath::from_locale_string("/test\x81\x82/foobar");

		SECTION("Path doesn't start with base") {
			const auto base = Filepath::from_locale_string("baz\x80quux");
			REQUIRE_FALSE(path.starts_with(base));
		}

		SECTION("Path starts with base") {
			const auto base = Filepath::from_locale_string("/test\x81\x82/");
			REQUIRE(path.starts_with(base));
		}
	}
}

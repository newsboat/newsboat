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

TEST_CASE("Can be cloned", "[Filepath]")
{
	auto original = Filepath::from_locale_string("/etc/hosts");
	const auto clone = original.clone();
	REQUIRE(original == clone);
	REQUIRE(original.display() == "/etc/hosts");

	// Demonstrate that changing the original object doesn't modify the clone
	original.push(Filepath::from_locale_string(" a bit more"));
	REQUIRE(original.display() == "/etc/hosts/ a bit more");
	REQUIRE(clone.display() == "/etc/hosts");
}

TEST_CASE("Set extension", "[Filepath]")
{
	Filepath path;
	REQUIRE_FALSE(path.set_extension("exe"));

	path.push("file");
	REQUIRE(path.set_extension("exe"));
	REQUIRE(path == "file.exe");
}
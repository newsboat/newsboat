#include "3rd-party/catch.hpp"

#include "filesystembrowser.h"

using namespace newsboat::FileSystemBrowser;

TEST_CASE("mode_suffix", "[FileSystemBrowser]")
{
	SECTION("Basic checks") {
		REQUIRE(mode_suffix(0644 | S_IFREG) == nonstd::nullopt);
		REQUIRE(mode_suffix(0644 | S_IFDIR) == '/');
		REQUIRE(mode_suffix(0644 | S_IFLNK) == '@');
		REQUIRE(mode_suffix(0644 | S_IFSOCK) == '=');
		REQUIRE(mode_suffix(0644 | S_IFIFO) == '|');
	}

	SECTION("Type is more important than executable bits") {
		REQUIRE(mode_suffix(0744 | S_IFDIR) == '/');
		REQUIRE(mode_suffix(0654 | S_IFDIR) == '/');
		REQUIRE(mode_suffix(0645 | S_IFDIR) == '/');

		REQUIRE(mode_suffix(0744 | S_IFLNK) == '@');
		REQUIRE(mode_suffix(0654 | S_IFLNK) == '@');
		REQUIRE(mode_suffix(0645 | S_IFLNK) == '@');

		REQUIRE(mode_suffix(0744 | S_IFSOCK) == '=');
		REQUIRE(mode_suffix(0654 | S_IFSOCK) == '=');
		REQUIRE(mode_suffix(0645 | S_IFSOCK) == '=');

		REQUIRE(mode_suffix(0744 | S_IFIFO) == '|');
		REQUIRE(mode_suffix(0654 | S_IFIFO) == '|');
		REQUIRE(mode_suffix(0645 | S_IFIFO) == '|');
	}

	SECTION("Owner executable bit results in asterisk for some types") {
		REQUIRE(mode_suffix(0744 | S_IFREG) == '*');
		REQUIRE(mode_suffix(0744 | S_IFBLK) == '*');
		REQUIRE(mode_suffix(0744 | S_IFCHR) == '*');

		// Group executable bit set => no asterisk
		REQUIRE(mode_suffix(0654 | S_IFREG) == nonstd::nullopt);
		REQUIRE(mode_suffix(0654 | S_IFBLK) == nonstd::nullopt);
		REQUIRE(mode_suffix(0654 | S_IFCHR) == nonstd::nullopt);

		// Other executable bit set => no asterisk
		REQUIRE(mode_suffix(0645 | S_IFREG) == nonstd::nullopt);
		REQUIRE(mode_suffix(0645 | S_IFBLK) == nonstd::nullopt);
		REQUIRE(mode_suffix(0645 | S_IFCHR) == nonstd::nullopt);
	}
}

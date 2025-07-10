#include "3rd-party/catch.hpp"

#include "file_system.h"

using namespace Newsboat::file_system;

TEST_CASE("mode_suffix", "[file_system]")
{
	SECTION("Basic checks") {
		REQUIRE(mode_suffix(0644 | S_IFREG) == std::nullopt);
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
		REQUIRE(mode_suffix(0654 | S_IFREG) == std::nullopt);
		REQUIRE(mode_suffix(0654 | S_IFBLK) == std::nullopt);
		REQUIRE(mode_suffix(0654 | S_IFCHR) == std::nullopt);

		// Other executable bit set => no asterisk
		REQUIRE(mode_suffix(0645 | S_IFREG) == std::nullopt);
		REQUIRE(mode_suffix(0645 | S_IFBLK) == std::nullopt);
		REQUIRE(mode_suffix(0645 | S_IFCHR) == std::nullopt);
	}
}

TEST_CASE("permissions_string", "[file_system]")
{
	REQUIRE(permissions_string(0710) == "rwx--x---");
	REQUIRE(permissions_string(0257) == "-w-r-xrwx");
	REQUIRE(permissions_string(0616) == "rw---xrw-");
	REQUIRE(permissions_string(0227) == "-w--w-rwx");
	REQUIRE(permissions_string(0006) == "------rw-");
	REQUIRE(permissions_string(0133) == "--x-wx-wx");
	REQUIRE(permissions_string(0346) == "-wxr--rw-");
	REQUIRE(permissions_string(0017) == "-----xrwx");
	REQUIRE(permissions_string(0254) == "-w-r-xr--");
	REQUIRE(permissions_string(0646) == "rw-r--rw-");
	REQUIRE(permissions_string(0326) == "-wx-w-rw-");
	REQUIRE(permissions_string(0754) == "rwxr-xr--");
	REQUIRE(permissions_string(0156) == "--xr-xrw-");
}

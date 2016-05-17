#include "catch.hpp"

#include <htmlrenderer.h>

using namespace newsbeuter;

TEST_CASE("HTMLRenderer behaves correctly") {
	htmlrenderer rnd(100);
	std::vector<std::string> lines;
	std::vector<linkpair> links;

	SECTION("link rendering") {
		rnd.render("<a href=\"http://slashdot.org/\">slashdot</a>", lines, links, "");

		REQUIRE(lines.size() == 4);

		REQUIRE(lines[0] == "<u>slashdot</>[1]");
		REQUIRE(lines[1] == "");
		REQUIRE(lines[2] == "Links: ");
		REQUIRE(lines[3] == "[1]: http://slashdot.org/ (link)");

		REQUIRE(links[0].first == "http://slashdot.org/");
		REQUIRE(links[0].second == LINK_HREF);

	}

	SECTION("line break rendering") {
		for (std::string tag : {"<br>", "<br/>", "<br />"}) {
			SECTION(tag) {
				rnd.render("hello" + tag + "world!", lines, links, "");
				REQUIRE(lines.size() == 2);
				REQUIRE(lines[0] == "hello");
				REQUIRE(lines[1] == "world!");
			}
		}
	}

	SECTION("superscript rendering") {
		rnd.render("3<sup>10</sup>", lines, links, "");
		REQUIRE(lines.size() == 1);
		REQUIRE(lines[0] == "3^10");
	}

	SECTION("subscript rendering") {
		rnd.render("A<sub>i</sub>", lines, links, "");
		REQUIRE(lines.size() == 1);
		REQUIRE(lines[0] == "A[i]");
	}

	SECTION("scripts are ignored") {
		rnd.render("abc<script></script>", lines, links, "");
		REQUIRE(lines.size() == 1);
		REQUIRE(lines[0] == "abc");
	}
}

TEST_CASE("htmlrenderer::format_ol_count()") {
	htmlrenderer r;

	SECTION("format to digit") {
		REQUIRE(r.format_ol_count(1, '1') == " 1");
		REQUIRE(r.format_ol_count(3, '1') == " 3");
	}

	SECTION("format to alphabetic") {
		REQUIRE(r.format_ol_count(3, 'a') == "c");
		REQUIRE(r.format_ol_count(26 + 3, 'a') == "ac");
		REQUIRE(r.format_ol_count(3*26*26 + 5*26 + 2, 'a') == "ceb");
	}

	SECTION("format to alphabetic uppercase") {
		REQUIRE(r.format_ol_count(3, 'A') == "C");
		REQUIRE(r.format_ol_count(26 + 5, 'A') == "AE");
		REQUIRE(r.format_ol_count(27, 'A') == "AA");
		REQUIRE(r.format_ol_count(26, 'A') == "Z");
		REQUIRE(r.format_ol_count(26*26+26, 'A') == "ZZ");
		REQUIRE(r.format_ol_count(25*26*26 + 26*26+26, 'A') == "YZZ");
	}

	SECTION("format to roman numerals") {
		REQUIRE(r.format_ol_count(1, 'i') == "i");
		REQUIRE(r.format_ol_count(2, 'i') == "ii");
		REQUIRE(r.format_ol_count(5, 'i') == "v");
		REQUIRE(r.format_ol_count(4, 'i') == "iv");
		REQUIRE(r.format_ol_count(6, 'i') == "vi");
		REQUIRE(r.format_ol_count(7, 'i') == "vii");
		REQUIRE(r.format_ol_count(10, 'i') == "x");
		REQUIRE(r.format_ol_count(32, 'i') == "xxxii");
		REQUIRE(r.format_ol_count(1972, 'i') == "mcmlxxii");
	}

	SECTION("format to uppercase roman numerals") {
		REQUIRE(r.format_ol_count(2011, 'I') == "MMXI");
	}
}

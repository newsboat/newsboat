#include "catch.hpp"

#include <sstream>

#include <htmlrenderer.h>
#include <utils.h>
#include <strprintf.h>

using namespace newsbeuter;

const std::string url = "http://example.com/feed.rss";

/*
 * We're going to do an awful lot of comparisons of these pairs, so it makes
 * sense to create a function to make constructing them easier.
 *
 * And while we're at it, let's make it accept char* instead of string so that
 * we don't have to wrap string literals in std::string() calls (we don't
 * support C++14 yet so can't use their fancy string literal operators.)
 */
std::pair<newsbeuter::LineType, std::string>
p(newsbeuter::LineType type, const char* str) {
	return std::make_pair(type, std::string(str));
}

namespace Catch {
	/* Catch doesn't know how to print out newsbeuter::LineType values, so
	 * let's teach it!
	 *
	 * Technically, any one of the following two definitions shouls be enough,
	 * but somehow it's not that way: toString works in std::pair<> while
	 * StringMaker is required for simple things like REQUIRE(LineType::hr ==
	 * LineType::hr). Weird, but at least this works.
	 */
	std::string toString(newsbeuter::LineType const& value) {
		switch(value) {
			case LineType::wrappable:
				return "wrappable";
			case LineType::softwrappable:
				return "softwrappable";
			case LineType::nonwrappable:
				return "nonwrappable";
			case LineType::hr:
				return "hr";
			default:
				return "(unknown LineType)";
		}
	}

	template<> struct StringMaker<newsbeuter::LineType> {
		static std::string convert(newsbeuter::LineType const& value) {
			return toString(value);
		}
	};

	// Catch also doesn't know about std::pair
	template<> struct StringMaker<std::pair<newsbeuter::LineType, std::string>> {
		static std::string convert(std::pair<newsbeuter::LineType, std::string> const& value ) {
			std::ostringstream o;
			o << "(";
			o << toString(value.first);
			o << ", \"";
			o << value.second;
			o << "\")";
			return o.str();
		}
	};
}

TEST_CASE("Links are rendered as underlined text with reference number in "
          "square brackets", "[htmlrenderer]")
{
	htmlrenderer rnd;
	std::vector<std::pair<LineType, std::string>> lines;
	std::vector<linkpair> links;

	rnd.render("<a href=\"http://slashdot.org/\">slashdot</a>", lines, links, "");

	REQUIRE(lines.size() == 4);

	REQUIRE(lines[0] == p(LineType::wrappable, "<u>slashdot</>[1]"));
	REQUIRE(lines[1] == p(LineType::wrappable, ""));
	REQUIRE(lines[2] == p(LineType::wrappable, "Links: "));
	REQUIRE(lines[3] == p(LineType::softwrappable, "[1]: http://slashdot.org/ (link)"));

	REQUIRE(links[0].first == "http://slashdot.org/");
	REQUIRE(links[0].second == link_type::HREF);
}

TEST_CASE("<br>, <br/> and <br /> result in a line break", "[htmlrenderer]") {
	htmlrenderer rnd;
	std::vector<std::pair<LineType, std::string>> lines;
	std::vector<linkpair> links;

	for (std::string tag : {"<br>", "<br/>", "<br />"}) {
		SECTION(tag) {
			auto input = "hello" + tag + "world!";
			rnd.render(input, lines, links, "");
			REQUIRE(lines.size() == 2);
			REQUIRE(lines[0] == p(LineType::wrappable, "hello"));
			REQUIRE(lines[1] == p(LineType::wrappable, "world!"));
		}
	}
}

TEST_CASE("Superscript is rendered with caret symbol", "[htmlrenderer]") {
	htmlrenderer rnd;
	std::vector<std::pair<LineType, std::string>> lines;
	std::vector<linkpair> links;

	rnd.render("3<sup>10</sup>", lines, links, "");
	REQUIRE(lines.size() == 1);
	REQUIRE(lines[0] == p(LineType::wrappable, "3^10"));
}

TEST_CASE("Subscript is rendered with square brackets", "[htmlrenderer]") {
	htmlrenderer rnd;
	std::vector<std::pair<LineType, std::string>> lines;
	std::vector<linkpair> links;

	rnd.render("A<sub>i</sub>", lines, links, "");
	REQUIRE(lines.size() == 1);
	REQUIRE(lines[0] == p(LineType::wrappable, "A[i]"));
}

TEST_CASE("Script tags are ignored", "[htmlrenderer]") {
	htmlrenderer rnd;
	std::vector<std::pair<LineType, std::string>> lines;
	std::vector<linkpair> links;

	rnd.render("abc<script></script>", lines, links, "");
	REQUIRE(lines.size() == 1);
	REQUIRE(lines[0] == p(LineType::wrappable, "abc"));
}

TEST_CASE("format_ol_count formats list count in specified format",
          "[htmlrenderer]") {
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

TEST_CASE("links with same URL are coalesced under one number",
          "[htmlrenderer]") {
	htmlrenderer r;

	const std::string input =
		"<a href='http://example.com/about'>About us</a>"
		"<a href='http://example.com/about'>Another link</a>";
	std::vector<std::pair<LineType, std::string>> lines;
	std::vector<linkpair> links;

	REQUIRE_NOTHROW(r.render(input, lines, links, url));
	REQUIRE(links.size() == 1);
	REQUIRE(links[0].first == "http://example.com/about");
	REQUIRE(links[0].second == link_type::HREF);
}

TEST_CASE("links with different URLs have different numbers", "[htmlrenderer]") {
	htmlrenderer r;

	const std::string input =
		"<a href='http://example.com/one'>One</a>"
		"<a href='http://example.com/two'>Two</a>";
	std::vector<std::pair<LineType, std::string>> lines;
	std::vector<linkpair> links;

	REQUIRE_NOTHROW(r.render(input, lines, links, url));
	REQUIRE(links.size() == 2);
	REQUIRE(links[0].first == "http://example.com/one");
	REQUIRE(links[0].second == link_type::HREF);
	REQUIRE(links[1].first == "http://example.com/two");
	REQUIRE(links[1].second == link_type::HREF);
}

TEST_CASE("link without `href' is neither highlighted nor added to links list",
          "[htmlrenderer]") {
	htmlrenderer r;

	const std::string input = "<a>test</a>";
	std::vector<std::pair<LineType, std::string>> lines;
	std::vector<linkpair> links;

	REQUIRE_NOTHROW(r.render(input, lines, links, url));
	REQUIRE(lines.size() == 1);
	REQUIRE(lines[0] == p(LineType::wrappable, "test"));
	REQUIRE(links.size() == 0);
}

TEST_CASE("link with empty `href' is neither highlighted nor added to links list",
          "[htmlrenderer]") {
	htmlrenderer r;

	const std::string input = "<a href=''>test</a>";
	std::vector<std::pair<LineType, std::string>> lines;
	std::vector<linkpair> links;

	REQUIRE_NOTHROW(r.render(input, lines, links, url));
	REQUIRE(lines.size() == 1);
	REQUIRE(lines[0] == p(LineType::wrappable, "test"));
	REQUIRE(links.size() == 0);
}

TEST_CASE("<strong> is rendered in bold font", "[htmlrenderer]") {
	htmlrenderer r;

	const std::string input = "<strong>test</strong>";
	std::vector<std::pair<LineType, std::string>> lines;
	std::vector<linkpair> links;

	REQUIRE_NOTHROW(r.render(input, lines, links, url));
	REQUIRE(lines.size() == 1);
	REQUIRE(lines[0] == p(LineType::wrappable, "<b>test</>"));
}

TEST_CASE("<u> is rendered as underlined text", "[htmlrenderer]") {
	htmlrenderer r;

	const std::string input = "<u>test</u>";
	std::vector<std::pair<LineType, std::string>> lines;
	std::vector<linkpair> links;

	REQUIRE_NOTHROW(r.render(input, lines, links, url));
	REQUIRE(lines.size() == 1);
	REQUIRE(lines[0] == p(LineType::wrappable, "<u>test</>"));
}

TEST_CASE("<q> is rendered as text in quotes", "[htmlrenderer]") {
	htmlrenderer r;

	const std::string input = "<q>test</q>";
	std::vector<std::pair<LineType, std::string>> lines;
	std::vector<linkpair> links;

	REQUIRE_NOTHROW(r.render(input, lines, links, url));
	REQUIRE(lines.size() == 1);
	REQUIRE(lines[0] == p(LineType::wrappable, "\"test\""));
}

TEST_CASE("Flash <embed>s are added to links if `src' is set", "[htmlrenderer]") {
	htmlrenderer r;

	const std::string input =
		"<embed type='application/x-shockwave-flash'"
			"src='http://example.com/game.swf'>"
		"</embed>";
	std::vector<std::pair<LineType, std::string>> lines;
	std::vector<linkpair> links;

	REQUIRE_NOTHROW(r.render(input, lines, links, url));
	REQUIRE(lines.size() == 4);
	REQUIRE(lines[0] == p(LineType::wrappable, "[embedded flash: 1]"));
	REQUIRE(lines[1] == p(LineType::wrappable, ""));
	REQUIRE(lines[2] == p(LineType::wrappable, "Links: "));
	REQUIRE(lines[3] == p(LineType::softwrappable, "[1]: http://example.com/game.swf (embedded flash)"));
	REQUIRE(links.size() == 1);
	REQUIRE(links[0].first == "http://example.com/game.swf");
	REQUIRE(links[0].second == link_type::EMBED);
}

TEST_CASE("Flash <embed>s are ignored if `src' is not set", "[htmlrenderer]") {
	htmlrenderer r;

	const std::string input =
		"<embed type='application/x-shockwave-flash'></embed>";
	std::vector<std::pair<LineType, std::string>> lines;
	std::vector<linkpair> links;

	REQUIRE_NOTHROW(r.render(input, lines, links, url));
	REQUIRE(lines.size() == 0);
	REQUIRE(links.size() == 0);
}

TEST_CASE("non-flash <embed>s are ignored", "[htmlrenderer]") {
	htmlrenderer r;

	const std::string input =
		"<embed type='whatever'"
			"src='http://example.com/thingy'></embed>";
	std::vector<std::pair<LineType, std::string>> lines;
	std::vector<linkpair> links;

	REQUIRE_NOTHROW(r.render(input, lines, links, url));
	REQUIRE(lines.size() == 0);
	REQUIRE(links.size() == 0);
}

TEST_CASE("<embed>s are ignored if `type' is not set", "[htmlrenderer]") {
	htmlrenderer r;

	const std::string input =
		"<embed src='http://example.com/yet.another.thingy'></embed>";
	std::vector<std::pair<LineType, std::string>> lines;
	std::vector<linkpair> links;

	REQUIRE_NOTHROW(r.render(input, lines, links, url));
	REQUIRE(lines.size() == 0);
	REQUIRE(links.size() == 0);
}

TEST_CASE("spaces and line breaks are preserved inside <pre>", "[htmlrenderer]") {
	htmlrenderer r;

	const std::string input =
		"<pre>oh cool\n"
		"  check this\tstuff  out!\n"
		"</pre>";
	std::vector<std::pair<LineType, std::string>> lines;
	std::vector<linkpair> links;

	REQUIRE_NOTHROW(r.render(input, lines, links, url));
	REQUIRE(lines.size() == 2);
	REQUIRE(lines[0] == p(LineType::softwrappable, "oh cool"));
	REQUIRE(lines[1] == p(LineType::softwrappable, "  check this\tstuff  out!"));
	REQUIRE(links.size() == 0);
}

TEST_CASE("tags still work inside <pre>", "[htmlrenderer]") {
	htmlrenderer r;

	const std::string input =
		"<pre>"
			"<strong>bold text</strong>"
			"<u>underlined text</u>"
		"</pre>";
	std::vector<std::pair<LineType, std::string>> lines;
	std::vector<linkpair> links;

	REQUIRE_NOTHROW(r.render(input, lines, links, url));
	REQUIRE(lines.size() == 1);
	REQUIRE(lines[0] == p(LineType::softwrappable, "<b>bold text</><u>underlined text</>"));
	REQUIRE(links.size() == 0);
}

TEST_CASE("<img> results in a placeholder and a link", "[htmlrenderer]") {
	htmlrenderer r;

	const std::string input =
		"<img src='http://example.com/image.png'></img>";
	std::vector<std::pair<LineType, std::string>> lines;
	std::vector<linkpair> links;

	REQUIRE_NOTHROW(r.render(input, lines, links, url));
	REQUIRE(lines.size() == 4);
	REQUIRE(lines[0] == p(LineType::wrappable, "[image 1]"));
	REQUIRE(lines[1] == p(LineType::wrappable, ""));
	REQUIRE(lines[2] == p(LineType::wrappable, "Links: "));
	REQUIRE(lines[3] == p(LineType::softwrappable, "[1]: http://example.com/image.png (image)"));
	REQUIRE(links.size() == 1);
	REQUIRE(links[0].first == "http://example.com/image.png");
	REQUIRE(links[0].second == link_type::IMG);
}

TEST_CASE("<img>s without `src' are ignored", "[htmlrenderer]") {
	htmlrenderer r;

	const std::string input =
		"<img></img>";
	std::vector<std::pair<LineType, std::string>> lines;
	std::vector<linkpair> links;

	REQUIRE_NOTHROW(r.render(input, lines, links, url));
	REQUIRE(lines.size() == 0);
	REQUIRE(links.size() == 0);
}

TEST_CASE("title is mentioned in placeholder if <img> has `title'",
          "[htmlrenderer]") {
	htmlrenderer r;

	const std::string input =
		"<img src='http://example.com/image.png'"
			"title='Just a test image'></img>";
	std::vector<std::pair<LineType, std::string>> lines;
	std::vector<linkpair> links;

	REQUIRE_NOTHROW(r.render(input, lines, links, url));
	REQUIRE(lines.size() == 4);
	REQUIRE(lines[0] == p(LineType::wrappable, "[image 1: Just a test image]"));
	REQUIRE(lines[1] == p(LineType::wrappable, ""));
	REQUIRE(lines[2] == p(LineType::wrappable, "Links: "));
	REQUIRE(lines[3] == p(LineType::softwrappable, "[1]: http://example.com/image.png (image)"));
	REQUIRE(links.size() == 1);
	REQUIRE(links[0].first == "http://example.com/image.png");
	REQUIRE(links[0].second == link_type::IMG);
}

TEST_CASE("URL of <img> with data inside `src' is replaced with string "
          "\"inline image\"", "[htmlrenderer]") {
	htmlrenderer r;

	const std::string input =
		"<img src='data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAUA"
		"AAAFCAYAAACNbyblAAAAHElEQVQI12P4//8/w38GIAXDIBKE0DHxgljNBAAO"
		"9TXL0Y4OHwAAAABJRU5ErkJggg==' alt='Red dot' />";
	std::vector<std::pair<LineType, std::string>> lines;
	std::vector<linkpair> links;

	REQUIRE_NOTHROW(r.render(input, lines, links, url));
	REQUIRE(lines.size() == 4);
	REQUIRE(lines[0] == p(LineType::wrappable, "[image 1]"));
	REQUIRE(lines[1] == p(LineType::wrappable, ""));
	REQUIRE(lines[2] == p(LineType::wrappable, "Links: "));
	REQUIRE(lines[3] == p(LineType::softwrappable, "[1]: inline image (image)"));
	REQUIRE(links.size() == 1);
	REQUIRE(links[0].first == "inline image");
	REQUIRE(links[0].second == link_type::IMG);
}

TEST_CASE("<blockquote> is indented and is separated by empty lines",
          "[htmlrenderer]") {
	htmlrenderer r;

	const std::string input =
		"<blockquote>"
			"Experience is what you get when you didn't get what you wanted. "
			"&mdash;Randy Pausch"
		"</blockquote>";
	std::vector<std::pair<LineType, std::string>> lines;
	std::vector<linkpair> links;

	REQUIRE_NOTHROW(r.render(input, lines, links, url));
	REQUIRE(lines.size() == 3);
	REQUIRE(lines[0] == p(LineType::wrappable, ""));
	REQUIRE(lines[1] == p(LineType::wrappable, "  Experience is what you get when you didn't get "
	                    "what you wanted. —Randy Pausch"));
	REQUIRE(lines[2] == p(LineType::wrappable, ""));
	REQUIRE(links.size() == 0);
}

TEST_CASE("<dl>, <dt> and <dd> are rendered as a set of paragraphs with term "
          "descriptions indented to the right", "[htmlrenderer]")
{
	htmlrenderer r;

	const std::string input =
		// Opinions are lifted from the "Monstrous Regiment" by Terry Pratchett
		"<dl>"
			"<dt>Coffee</dt>"
			"<dd>Foul muck</dd>"
			"<dt>Tea</dt>"
			"<dd>Soldier's friend</dd>"
		"</dl>";
	std::vector<std::pair<LineType, std::string>> lines;
	std::vector<linkpair> links;

	REQUIRE_NOTHROW(r.render(input, lines, links, url));
	REQUIRE(lines.size() == 8);
	REQUIRE(lines[0] == p(LineType::wrappable, "Coffee"));
	REQUIRE(lines[1] == p(LineType::wrappable, ""));
	REQUIRE(lines[2] == p(LineType::wrappable, "        Foul muck"));
	REQUIRE(lines[3] == p(LineType::wrappable, ""));
	REQUIRE(lines[4] == p(LineType::wrappable, "Tea"));
	REQUIRE(lines[5] == p(LineType::wrappable, ""));
	REQUIRE(lines[6] == p(LineType::wrappable, "        Soldier's friend"));
	REQUIRE(lines[7] == p(LineType::wrappable, ""));
	REQUIRE(links.size() == 0);
}

TEST_CASE("<h[2-6]> and <p>", "[htmlrenderer]") {
	htmlrenderer r;
	std::vector<std::pair<LineType, std::string>> lines;
	std::vector<linkpair> links;

	SECTION("<h1> is rendered with setext-style underlining") {
		const std::string input = "<h1>Why are we here?</h1>";

		REQUIRE_NOTHROW(r.render(input, lines, links, url));
		REQUIRE(lines.size() == 2);
		REQUIRE(lines[0] == p(LineType::wrappable, "Why are we here?"));
		REQUIRE(lines[1] == p(LineType::wrappable, "----------------"));
		REQUIRE(links.size() == 0);
	}

	for (auto tag : {"<h2>", "<h3>", "<h4>", "<h5>", "<h6>", "<p>"}) {
		SECTION(std::string("When alone, ") + tag + " generates only one line") {
			std::string closing_tag = tag;
			closing_tag.insert(1, "/");

			const std::string input =
				tag + std::string("hello world") + closing_tag;
			REQUIRE_NOTHROW(r.render(input, lines, links, url));
			REQUIRE(lines.size() == 1);
			REQUIRE(lines[0] == p(LineType::wrappable, "hello world"));
			REQUIRE(links.size() == 0);
		}
	}

	SECTION("There's always an empty line between header/paragraph/list and paragraph")
	{
		SECTION("<h1>") {
			const std::string input = "<h1>header</h1><p>paragraph</p>";
			REQUIRE_NOTHROW(r.render(input, lines, links, url));
			REQUIRE(lines.size() == 4);
			REQUIRE(lines[2] == p(LineType::wrappable, ""));
			REQUIRE(links.size() == 0);
		}

		for (auto tag : {"<h2>", "<h3>", "<h4>", "<h5>", "<h6>", "<p>"}) {
			SECTION(tag) {
				std::string closing_tag = tag;
				closing_tag.insert(1, "/");

				const std::string input =
					tag + std::string("header") + closing_tag +
					"<p>paragraph</p>";

				REQUIRE_NOTHROW(r.render(input, lines, links, url));
				REQUIRE(lines.size() == 3);
				REQUIRE(lines[1] == p(LineType::wrappable, ""));
				REQUIRE(links.size() == 0);
			}
		}

		SECTION("<ul>") {
			const std::string input =
				"<ul><li>one</li><li>two</li></ul><p>paragraph</p>";
			REQUIRE_NOTHROW(r.render(input, lines, links, url));
			REQUIRE(lines.size() == 5);
			REQUIRE(lines[3] == p(LineType::wrappable, ""));
			REQUIRE(links.size() == 0);
		}

		SECTION("<ol>") {
			const std::string input =
				"<ol><li>one</li><li>two</li></ol><p>paragraph</p>";
			REQUIRE_NOTHROW(r.render(input, lines, links, url));
			REQUIRE(lines.size() == 5);
			REQUIRE(lines[3] == p(LineType::wrappable, ""));
			REQUIRE(links.size() == 0);
		}
	}
}

TEST_CASE("whitespace is erased at the beginning of the paragraph",
          "[htmlrenderer]") {
	htmlrenderer r;

	const std::string input =
		"<p>"
			"     \n"
			"here comes the text"
		"</p>";
	std::vector<std::pair<LineType, std::string>> lines;
	std::vector<linkpair> links;

	REQUIRE_NOTHROW(r.render(input, lines, links, url));
	REQUIRE(lines.size() == 1);
	REQUIRE(lines[0] == p(LineType::wrappable, "here comes the text"));
	REQUIRE(links.size() == 0);
}

TEST_CASE("newlines are replaced with space", "[htmlrenderer]") {
	htmlrenderer r;

	const std::string input = "newlines\nshould\nbe\nreplaced\nwith\na\nspace\ncharacter.";
	std::vector<std::pair<LineType, std::string>> lines;
	std::vector<linkpair> links;

	REQUIRE_NOTHROW(r.render(input, lines, links, url));
	REQUIRE(lines.size() == 1);
	REQUIRE(lines[0] == p(LineType::wrappable, "newlines should be replaced with a space character."));
	REQUIRE(links.size() == 0);
}

TEST_CASE("paragraph is just a long line of text", "[htmlrenderer]") {
	htmlrenderer r;

	const std::string input =
		"<p>"
			"here comes a long, boring chunk text that we have to fit to width"
		"</p>";
	std::vector<std::pair<LineType, std::string>> lines;
	std::vector<linkpair> links;

	REQUIRE_NOTHROW(r.render(input, lines, links, url));
	REQUIRE(lines.size() == 1);
	REQUIRE(lines[0] == p(LineType::wrappable, "here comes a long, boring chunk text "
				"that we have to fit to width"));
	REQUIRE(links.size() == 0);
}

TEST_CASE("default style for <ol> is Arabic numerals", "[htmlrenderer]") {
	htmlrenderer r;
	std::vector<std::pair<LineType, std::string>> lines;
	std::vector<linkpair> links;

	SECTION("no `type' attribute") {
		const std::string input =
			"<ol>"
				"<li>one</li>"
				"<li>two</li>"
			"</ol>";

		REQUIRE_NOTHROW(r.render(input, lines, links, url));
		REQUIRE(lines.size() == 4);
		REQUIRE(lines[1] == p(LineType::wrappable, " 1. one"));
		REQUIRE(lines[2] == p(LineType::wrappable, " 2. two"));
		REQUIRE(links.size() == 0);
	}

	SECTION("invalid `type' attribute") {
		const std::string input =
			"<ol type='invalid value'>"
				"<li>one</li>"
				"<li>two</li>"
			"</ol>";

		REQUIRE_NOTHROW(r.render(input, lines, links, url));
		REQUIRE(lines.size() == 4);
		REQUIRE(lines[1] == p(LineType::wrappable, " 1. one"));
		REQUIRE(lines[2] == p(LineType::wrappable, " 2. two"));
		REQUIRE(links.size() == 0);
	}
}

TEST_CASE("default starting number for <ol> is 1", "[htmlrenderer]") {
	htmlrenderer r;
	std::vector<std::pair<LineType, std::string>> lines;
	std::vector<linkpair> links;

	SECTION("no `start' attribute") {
		const std::string input =
			"<ol>"
				"<li>one</li>"
				"<li>two</li>"
			"</ol>";

		REQUIRE_NOTHROW(r.render(input, lines, links, url));
		REQUIRE(lines.size() == 4);
		REQUIRE(lines[1] == p(LineType::wrappable, " 1. one"));
		REQUIRE(lines[2] == p(LineType::wrappable, " 2. two"));
		REQUIRE(links.size() == 0);
	}

	SECTION("invalid `start' attribute") {
		const std::string input =
			"<ol start='whatever'>"
				"<li>one</li>"
				"<li>two</li>"
			"</ol>";

		REQUIRE_NOTHROW(r.render(input, lines, links, url));
		REQUIRE(lines.size() == 4);
		REQUIRE(lines[1] == p(LineType::wrappable, " 1. one"));
		REQUIRE(lines[2] == p(LineType::wrappable, " 2. two"));
		REQUIRE(links.size() == 0);
	}
}

TEST_CASE("type='1' for <ol> means Arabic numbering", "[htmlrenderer]") {
	htmlrenderer r;
	std::vector<std::pair<LineType, std::string>> lines;
	std::vector<linkpair> links;
	const std::string input =
		"<ol type='1'>"
			"<li>one</li>"
			"<li>two</li>"
		"</ol>";

	REQUIRE_NOTHROW(r.render(input, lines, links, url));
	REQUIRE(lines.size() == 4);
	REQUIRE(lines[1] == p(LineType::wrappable, " 1. one"));
	REQUIRE(lines[2] == p(LineType::wrappable, " 2. two"));
	REQUIRE(links.size() == 0);
}

TEST_CASE("type='a' for <ol> means lowercase alphabetic numbering",
          "[htmlrenderer]") {
	htmlrenderer r;
	std::vector<std::pair<LineType, std::string>> lines;
	std::vector<linkpair> links;
	const std::string input =
		"<ol type='a'>"
			"<li>one</li>"
			"<li>two</li>"
		"</ol>";

	REQUIRE_NOTHROW(r.render(input, lines, links, url));
	REQUIRE(lines.size() == 4);
	REQUIRE(lines[1] == p(LineType::wrappable, "a. one"));
	REQUIRE(lines[2] == p(LineType::wrappable, "b. two"));
	REQUIRE(links.size() == 0);
}

TEST_CASE("type='A' for <ol> means uppercase alphabetic numbering",
          "[htmlrenderer]") {
	htmlrenderer r;
	std::vector<std::pair<LineType, std::string>> lines;
	std::vector<linkpair> links;
	const std::string input =
		"<ol type='A'>"
			"<li>one</li>"
			"<li>two</li>"
		"</ol>";

	REQUIRE_NOTHROW(r.render(input, lines, links, url));
	REQUIRE(lines.size() == 4);
	REQUIRE(lines[1] == p(LineType::wrappable, "A. one"));
	REQUIRE(lines[2] == p(LineType::wrappable, "B. two"));
	REQUIRE(links.size() == 0);
}

TEST_CASE("type='i' for <ol> means lowercase Roman numbering",
          "[htmlrenderer]") {
	htmlrenderer r;
	std::vector<std::pair<LineType, std::string>> lines;
	std::vector<linkpair> links;
	const std::string input =
		"<ol type='i'>"
			"<li>one</li>"
			"<li>two</li>"
		"</ol>";

	REQUIRE_NOTHROW(r.render(input, lines, links, url));
	REQUIRE(lines.size() == 4);
	REQUIRE(lines[1] == p(LineType::wrappable, "i. one"));
	REQUIRE(lines[2] == p(LineType::wrappable, "ii. two"));
	REQUIRE(links.size() == 0);
}

TEST_CASE("type='I' for <ol> means uppercase Roman numbering",
          "[htmlrenderer]") {
	htmlrenderer r;
	std::vector<std::pair<LineType, std::string>> lines;
	std::vector<linkpair> links;
	const std::string input =
		"<ol type='I'>"
			"<li>one</li>"
			"<li>two</li>"
		"</ol>";

	REQUIRE_NOTHROW(r.render(input, lines, links, url));
	REQUIRE(lines.size() == 4);
	REQUIRE(lines[1] == p(LineType::wrappable, "I. one"));
	REQUIRE(lines[2] == p(LineType::wrappable, "II. two"));
	REQUIRE(links.size() == 0);
}

TEST_CASE("every next <li> implicitly closes the previous one",
          "[htmlrenderer]") {
	htmlrenderer r;
	std::vector<std::pair<LineType, std::string>> lines;
	std::vector<linkpair> links;
	const std::string input =
		"<ol type='I'>"
			"<li>one"
				"<li>two</li>"
				"<li>three</li>"
			"</li>"
			"<li>four</li>"
		"</ol>";

	REQUIRE_NOTHROW(r.render(input, lines, links, url));
	REQUIRE(lines.size() == 6);
	REQUIRE(lines[1] == p(LineType::wrappable, "I. one"));
	REQUIRE(lines[2] == p(LineType::wrappable, "II. two"));
	REQUIRE(lines[3] == p(LineType::wrappable, "III. three"));
	REQUIRE(lines[4] == p(LineType::wrappable, "IV. four"));
	REQUIRE(links.size() == 0);
}

TEST_CASE("<style> tags are ignored", "[htmlrenderer]") {
	htmlrenderer r;
	std::vector<std::pair<LineType, std::string>> lines;
	std::vector<linkpair> links;
	const std::string input =
		"<style><h1>ignore me</h1><p>and me</p> body{width: 100%;}</style>";

	REQUIRE_NOTHROW(r.render(input, lines, links, url));
	REQUIRE(lines.size() == 0);
	REQUIRE(links.size() == 0);
}

TEST_CASE("<hr> is not a string, but a special type of line",
          "[htmlrenderer]") {
	std::vector<std::pair<LineType, std::string>> lines;
	std::vector<linkpair> links;
	htmlrenderer r;
	const std::string input = "<hr>";

	REQUIRE_NOTHROW(r.render(input, lines, links, url));
	REQUIRE(links.size() == 0);

	REQUIRE(lines.size() == 1);
	REQUIRE(lines[0].first == LineType::hr);
}

TEST_CASE("header rows of tables are in bold", "[htmlrenderer]") {
	htmlrenderer r;
	std::vector<std::pair<LineType, std::string>> lines;
	std::vector<linkpair> links;

	SECTION("one column") {
		const std::string input =
			"<table>"
				"<tr>"
					"<th>header</th>"
				"</tr>"
			"</table>";

		REQUIRE_NOTHROW(r.render(input, lines, links, url));
		REQUIRE(lines.size() == 1);
		REQUIRE(lines[0] == p(LineType::nonwrappable, "<b>header</>"));
		REQUIRE(links.size() == 0);
	}

	SECTION("two columns") {
		const std::string input =
			"<table>"
				"<tr>"
					"<th>another</th>"
					"<th>header</th>"
				"</tr>"
			"</table>";

		REQUIRE_NOTHROW(r.render(input, lines, links, url));
		REQUIRE(lines.size() == 1);
		REQUIRE(lines[0] == p(LineType::nonwrappable, "<b>another</> <b>header</>"));
		REQUIRE(links.size() == 0);
	}
}

TEST_CASE("cells are separated by space if `border' is not set",
          "[htmlrenderer]") {
	htmlrenderer r;
	std::vector<std::pair<LineType, std::string>> lines;
	std::vector<linkpair> links;

	const std::string input =
		"<table>"
			"<tr>"
				"<td>hello</td>"
				"<td>world</td>"
			"</tr>"
		"</table>";

	REQUIRE_NOTHROW(r.render(input, lines, links, url));
	REQUIRE(lines.size() == 1);
	REQUIRE(lines[0] == p(LineType::nonwrappable, "hello world"));
	REQUIRE(links.size() == 0);
}

TEST_CASE("cells are separated by vertical bar if `border' is set (regardless "
          "of actual value)", "[htmlrenderer]") {
	htmlrenderer r;
	std::vector<std::pair<LineType, std::string>> lines;
	std::vector<linkpair> links;

	for (auto border_width = 1; border_width < 10; ++border_width) {
		SECTION(strprintf::fmt("`border' = %u", border_width)) {
		const std::string input_template =
			"<table border='%u'>"
				"<tr>"
					"<td>hello</td>"
					"<td>world</td>"
				"</tr>"
			"</table>";
		const std::string input =
			strprintf::fmt(input_template, border_width);

		REQUIRE_NOTHROW(r.render(input, lines, links, url));
		REQUIRE(lines.size() == 3);
		REQUIRE(lines[1] == p(LineType::nonwrappable, "|hello|world|"));
		REQUIRE(links.size() == 0);
		}
	}
}

TEST_CASE("tables with `border' have borders", "[htmlrenderer]") {
	htmlrenderer r;
	std::vector<std::pair<LineType, std::string>> lines;
	std::vector<linkpair> links;

	const std::string input =
		"<table border='1'>"
			"<tr>"
				"<td>hello</td>"
				"<td>world</td>"
			"</tr>"
		"</table>";

	REQUIRE_NOTHROW(r.render(input, lines, links, url));
	REQUIRE(lines.size() == 3);
	REQUIRE(lines[0] == p(LineType::nonwrappable, "+-----+-----+"));
	REQUIRE(lines[1] == p(LineType::nonwrappable, "|hello|world|"));
	REQUIRE(lines[2] == p(LineType::nonwrappable, "+-----+-----+"));
	REQUIRE(links.size() == 0);
}

TEST_CASE("if document ends before </table> is found, table is rendered anyway",
          "[htmlrenderer]") {
	htmlrenderer r;
	std::vector<std::pair<LineType, std::string>> lines;
	std::vector<linkpair> links;

	const std::string input =
		"<table border='1'>"
			"<tr>"
				"<td>hello</td>"
				"<td>world</td>"
			"</tr>";

	REQUIRE_NOTHROW(r.render(input, lines, links, url));
	REQUIRE(lines.size() == 3);
	REQUIRE(lines[0] == p(LineType::nonwrappable, "+-----+-----+"));
	REQUIRE(lines[1] == p(LineType::nonwrappable, "|hello|world|"));
	REQUIRE(lines[2] == p(LineType::nonwrappable, "+-----+-----+"));
	REQUIRE(links.size() == 0);
}

TEST_CASE("tables can be nested", "[htmlrenderer]") {
	htmlrenderer r;
	std::vector<std::pair<LineType, std::string>> lines;
	std::vector<linkpair> links;

	const std::string input =
		"<table border='1'>"
			"<tr>"
				"<td>"
					"<table border='1'>"
						"<tr>"
							"<td>hello</td>"
							"<td>world</td>"
						"</tr>"
						"<tr>"
							"<td>another</td>"
							"<td>row</td>"
						"</tr>"
					"</table>"
				"</td>"
				"<td>lonely cell</td>"
			"</tr>"
		"</table>";

	REQUIRE_NOTHROW(r.render(input, lines, links, url));
	REQUIRE(lines.size() == 7);
	REQUIRE(lines[0] == p(LineType::nonwrappable, "+---------------+-----------+"));
	REQUIRE(lines[1] == p(LineType::nonwrappable, "|+-------+-----+|lonely cell|"));
	REQUIRE(lines[2] == p(LineType::nonwrappable, "||hello  |world||           |"));
	REQUIRE(lines[3] == p(LineType::nonwrappable, "|+-------+-----+|           |"));
	REQUIRE(lines[4] == p(LineType::nonwrappable, "||another|row  ||           |"));
	REQUIRE(lines[5] == p(LineType::nonwrappable, "|+-------+-----+|           |"));
	REQUIRE(lines[6] == p(LineType::nonwrappable, "+---------------+-----------+"));

	REQUIRE(links.size() == 0);
}

TEST_CASE("if <td> appears inside table but outside of a row, one is created "
          "implicitly", "[htmlrenderer]") {
	htmlrenderer r;
	std::vector<std::pair<LineType, std::string>> lines;
	std::vector<linkpair> links;

	const std::string input =
		"<table border='1'>"
				"<td>hello</td>"
				"<td>world</td>"
			"</tr>"
		"</table>";

	REQUIRE_NOTHROW(r.render(input, lines, links, url));
	REQUIRE(lines.size() == 3);
	REQUIRE(lines[0] == p(LineType::nonwrappable, "+-----+-----+"));
	REQUIRE(lines[1] == p(LineType::nonwrappable, "|hello|world|"));
	REQUIRE(lines[2] == p(LineType::nonwrappable, "+-----+-----+"));
	REQUIRE(links.size() == 0);
}

TEST_CASE("previous row is implicitly closed when <tr> is found",
          "[htmlrenderer]") {
	htmlrenderer r;
	std::vector<std::pair<LineType, std::string>> lines;
	std::vector<linkpair> links;

	const std::string input =
		"<table border='1'>"
			"<tr><td>hello</td>"
			"<tr><td>world</td>"
		"</table>";

	REQUIRE_NOTHROW(r.render(input, lines, links, url));
	REQUIRE(lines.size() == 5);
	REQUIRE(lines[0] == p(LineType::nonwrappable, "+-----+"));
	REQUIRE(lines[1] == p(LineType::nonwrappable, "|hello|"));
	REQUIRE(lines[2] == p(LineType::nonwrappable, "+-----+"));
	REQUIRE(lines[3] == p(LineType::nonwrappable, "|world|"));
	REQUIRE(lines[4] == p(LineType::nonwrappable, "+-----+"));
	REQUIRE(links.size() == 0);
}

TEST_CASE("free-standing text outside of <td> is implicitly concatenated with "
          "the next cell", "[htmlrenderer]") {
	htmlrenderer r;
	std::vector<std::pair<LineType, std::string>> lines;
	std::vector<linkpair> links;

	const std::string input =
		"<table border='1'>"
				"hello"
				"<td> world</td>"
			"</tr>"
		"</table>";

	REQUIRE_NOTHROW(r.render(input, lines, links, url));
	REQUIRE(lines.size() == 3);
	REQUIRE(lines[0] == p(LineType::nonwrappable, "+-----------+"));
	REQUIRE(lines[1] == p(LineType::nonwrappable, "|hello world|"));
	REQUIRE(lines[2] == p(LineType::nonwrappable, "+-----------+"));
	REQUIRE(links.size() == 0);
}

TEST_CASE("text within <ituneshack> is to be treated specially",
          "[htmlrenderer]") {
	htmlrenderer r;
	std::vector<std::pair<LineType, std::string>> lines;
	std::vector<linkpair> links;

	const std::string input =
		"<ituneshack>"
			"hello world!\n"
			"I'm a description from an iTunes feed. "
			"Apple just puts plain text into &lt;summary&gt; tag."
		"</ituneshack>";

	REQUIRE_NOTHROW(r.render(input, lines, links, url));
	REQUIRE(lines.size() == 2);
	REQUIRE(lines[0] == p(LineType::wrappable, "hello world!"));
	REQUIRE(lines[1]
			==
			p(LineType::wrappable, "I'm a description from an iTunes feed. Apple just "
				"puts plain text into <>summary> tag."));
	REQUIRE(links.size() == 0);
}

#include "catch.hpp"

#include <sstream>

#include <htmlrenderer.h>
#include <utils.h>

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

TEST_CASE("HTMLRenderer behaves correctly") {
	htmlrenderer rnd;
	std::vector<std::pair<LineType, std::string>> lines;
	std::vector<linkpair> links;

	SECTION("link rendering") {
		rnd.render("<a href=\"http://slashdot.org/\">slashdot</a>", lines, links, "");

		REQUIRE(lines.size() == 4);

		REQUIRE(lines[0] == p(LineType::wrappable, "<u>slashdot</>[1]"));
		REQUIRE(lines[1] == p(LineType::wrappable, ""));
		REQUIRE(lines[2] == p(LineType::wrappable, "Links: "));
		REQUIRE(lines[3] == p(LineType::nonwrappable, "[1]: http://slashdot.org/ (link)"));

		REQUIRE(links[0].first == "http://slashdot.org/");
		REQUIRE(links[0].second == link_type::HREF);

	}

	SECTION("line break rendering") {
		for (std::string tag : {"<br>", "<br/>", "<br />"}) {
			SECTION(tag) {
				rnd.render("hello" + tag + "world!", lines, links, "");
				REQUIRE(lines.size() == 2);
				REQUIRE(lines[0] == p(LineType::wrappable, "hello"));
				REQUIRE(lines[1] == p(LineType::wrappable, "world!"));
			}
		}
	}

	SECTION("superscript rendering") {
		rnd.render("3<sup>10</sup>", lines, links, "");
		REQUIRE(lines.size() == 1);
		REQUIRE(lines[0] == p(LineType::wrappable, "3^10"));
	}

	SECTION("subscript rendering") {
		rnd.render("A<sub>i</sub>", lines, links, "");
		REQUIRE(lines.size() == 1);
		REQUIRE(lines[0] == p(LineType::wrappable, "A[i]"));
	}

	SECTION("scripts are ignored") {
		rnd.render("abc<script></script>", lines, links, "");
		REQUIRE(lines.size() == 1);
		REQUIRE(lines[0] == p(LineType::wrappable, "abc"));
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

TEST_CASE("htmlrenderer: links with same URL are coalesced under one number") {
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

TEST_CASE("htmlrenderer: links with different URLs have different numbers") {
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

TEST_CASE("htmlrenderer: link without `href' is neither highlighted nor added to links list") {
	htmlrenderer r;

	const std::string input = "<a>test</a>";
	std::vector<std::pair<LineType, std::string>> lines;
	std::vector<linkpair> links;

	REQUIRE_NOTHROW(r.render(input, lines, links, url));
	REQUIRE(lines.size() == 1);
	REQUIRE(lines[0] == p(LineType::wrappable, "test"));
	REQUIRE(links.size() == 0);
}

TEST_CASE("htmlrenderer: link with empty `href' is neither highlighted nor added to links list") {
	htmlrenderer r;

	const std::string input = "<a href=''>test</a>";
	std::vector<std::pair<LineType, std::string>> lines;
	std::vector<linkpair> links;

	REQUIRE_NOTHROW(r.render(input, lines, links, url));
	REQUIRE(lines.size() == 1);
	REQUIRE(lines[0] == p(LineType::wrappable, "test"));
	REQUIRE(links.size() == 0);
}

TEST_CASE("htmlrenderer: <strong> is rendered in bold font") {
	htmlrenderer r;

	const std::string input = "<strong>test</strong>";
	std::vector<std::pair<LineType, std::string>> lines;
	std::vector<linkpair> links;

	REQUIRE_NOTHROW(r.render(input, lines, links, url));
	REQUIRE(lines.size() == 1);
	REQUIRE(lines[0] == p(LineType::wrappable, "<b>test</>"));
}

TEST_CASE("htmlrenderer: <u> is rendered as underlined text") {
	htmlrenderer r;

	const std::string input = "<u>test</u>";
	std::vector<std::pair<LineType, std::string>> lines;
	std::vector<linkpair> links;

	REQUIRE_NOTHROW(r.render(input, lines, links, url));
	REQUIRE(lines.size() == 1);
	REQUIRE(lines[0] == p(LineType::wrappable, "<u>test</>"));
}

TEST_CASE("htmlrenderer: <q> is rendered as text in quotes") {
	htmlrenderer r;

	const std::string input = "<q>test</q>";
	std::vector<std::pair<LineType, std::string>> lines;
	std::vector<linkpair> links;

	REQUIRE_NOTHROW(r.render(input, lines, links, url));
	REQUIRE(lines.size() == 1);
	REQUIRE(lines[0] == p(LineType::wrappable, "\"test\""));
}

TEST_CASE("htmlrenderer: Flash <embed>s are added to links if `src' is set") {
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
	REQUIRE(lines[3] == p(LineType::nonwrappable, "[1]: http://example.com/game.swf (embedded flash)"));
	REQUIRE(links.size() == 1);
	REQUIRE(links[0].first == "http://example.com/game.swf");
	REQUIRE(links[0].second == link_type::EMBED);
}

TEST_CASE("htmlrenderer: Flash <embed>s are ignored if `src' is not set") {
	htmlrenderer r;

	const std::string input =
		"<embed type='application/x-shockwave-flash'></embed>";
	std::vector<std::pair<LineType, std::string>> lines;
	std::vector<linkpair> links;

	REQUIRE_NOTHROW(r.render(input, lines, links, url));
	REQUIRE(lines.size() == 0);
	REQUIRE(links.size() == 0);
}

TEST_CASE("htmlrenderer: non-flash <embed>s are ignored") {
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

TEST_CASE("htmlrenderer: <embed>s are ignored if `type' is not set") {
	htmlrenderer r;

	const std::string input =
		"<embed src='http://example.com/yet.another.thingy'></embed>";
	std::vector<std::pair<LineType, std::string>> lines;
	std::vector<linkpair> links;

	REQUIRE_NOTHROW(r.render(input, lines, links, url));
	REQUIRE(lines.size() == 0);
	REQUIRE(links.size() == 0);
}

TEST_CASE("htmlrenderer: spaces and line breaks are preserved inside <pre>") {
	htmlrenderer r;

	const std::string input =
		"<pre>oh cool\n"
		"  check this\tstuff  out!\n"
		"</pre>";
	std::vector<std::pair<LineType, std::string>> lines;
	std::vector<linkpair> links;

	REQUIRE_NOTHROW(r.render(input, lines, links, url));
	REQUIRE(lines.size() == 2);
	REQUIRE(lines[0] == p(LineType::nonwrappable, "oh cool"));
	REQUIRE(lines[1] == p(LineType::nonwrappable, "  check this\tstuff  out!"));
	REQUIRE(links.size() == 0);
}

TEST_CASE("htmlrenderer: tags still work inside <pre>") {
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
	REQUIRE(lines[0] == p(LineType::nonwrappable, "<b>bold text</><u>underlined text</>"));
	REQUIRE(links.size() == 0);
}

TEST_CASE("htmlrenderer: <img> results in a placeholder and a link") {
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
	REQUIRE(lines[3] == p(LineType::nonwrappable, "[1]: http://example.com/image.png (image)"));
	REQUIRE(links.size() == 1);
	REQUIRE(links[0].first == "http://example.com/image.png");
	REQUIRE(links[0].second == link_type::IMG);
}

TEST_CASE("htmlrenderer: <img>s without `src' are ignored") {
	htmlrenderer r;

	const std::string input =
		"<img></img>";
	std::vector<std::pair<LineType, std::string>> lines;
	std::vector<linkpair> links;

	REQUIRE_NOTHROW(r.render(input, lines, links, url));
	REQUIRE(lines.size() == 0);
	REQUIRE(links.size() == 0);
}

TEST_CASE("htmlrenderer: title is mentioned in placeholder if <img> has `title'") {
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
	REQUIRE(lines[3] == p(LineType::nonwrappable, "[1]: http://example.com/image.png (image)"));
	REQUIRE(links.size() == 1);
	REQUIRE(links[0].first == "http://example.com/image.png");
	REQUIRE(links[0].second == link_type::IMG);
}

TEST_CASE("htmlrenderer: URL of <img> with data inside `src' is replaced with string \"inline image\"") {
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
	REQUIRE(lines[3] == p(LineType::nonwrappable, "[1]: inline image (image)"));
	REQUIRE(links.size() == 1);
	REQUIRE(links[0].first == "inline image");
	REQUIRE(links[0].second == link_type::IMG);
}

TEST_CASE("htmlrenderer: <blockquote> is indented and is separated by empty lines") {
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

TEST_CASE("htmlrenderer: <dl>, <dt> and <dd> are rendered properly") {
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

TEST_CASE("htmlrenderer: <h[2-6]> and <p>") {
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

TEST_CASE("htmlrenderer: whitespace is erased at the beginning of the paragraph") {
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

TEST_CASE("htmlrenderer: newlines are replaced with space") {
	htmlrenderer r;

	const std::string input = "newlines\nshould\nbe\nreplaced\nwith\na\nspace\ncharacter.";
	std::vector<std::pair<LineType, std::string>> lines;
	std::vector<linkpair> links;

	REQUIRE_NOTHROW(r.render(input, lines, links, url));
	REQUIRE(lines.size() == 1);
	REQUIRE(lines[0] == p(LineType::wrappable, "newlines should be replaced with a space character."));
	REQUIRE(links.size() == 0);
}

TEST_CASE("htmlrenderer: paragraph is just a long line of text") {
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

TEST_CASE("htmlrenderer: default style for <ol> is Arabic numerals") {
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

TEST_CASE("htmlrenderer: default starting number for <ol> is 1") {
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

TEST_CASE("htmlrenderer: type='1' for <ol> means Arabic numbering") {
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

TEST_CASE("htmlrenderer: type='a' for <ol> means lowercase alphabetic numbering") {
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

TEST_CASE("htmlrenderer: type='A' for <ol> means uppercase alphabetic numbering") {
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

TEST_CASE("htmlrenderer: type='i' for <ol> means lowercase Roman numbering") {
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

TEST_CASE("htmlrenderer: type='I' for <ol> means uppercase Roman numbering") {
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

TEST_CASE("htmlrenderer: every next <li> implicitly closes the previous one") {
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

TEST_CASE("htmlrenderer: <style> tags are ignored") {
	htmlrenderer r;
	std::vector<std::pair<LineType, std::string>> lines;
	std::vector<linkpair> links;
	const std::string input =
		"<style><h1>ignore me</h1><p>and me</p> body{width: 100%;}</style>";

	REQUIRE_NOTHROW(r.render(input, lines, links, url));
	REQUIRE(lines.size() == 0);
	REQUIRE(links.size() == 0);
}

TEST_CASE("htmlrenderer: <hr> is not a string, but a special type of line") {
	std::vector<std::pair<LineType, std::string>> lines;
	std::vector<linkpair> links;
	htmlrenderer r;
	const std::string input = "<hr>";

	REQUIRE_NOTHROW(r.render(input, lines, links, url));
	REQUIRE(links.size() == 0);

	REQUIRE(lines.size() == 1);
	REQUIRE(lines[0].first == LineType::hr);
}

TEST_CASE("htmlrenderer: header rows of tables are in bold") {
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
		REQUIRE(lines[0] == p(LineType::wrappable, "<b>header</>"));
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
		REQUIRE(lines[0] == p(LineType::wrappable, "<b>another</> <b>header</>"));
		REQUIRE(links.size() == 0);
	}
}

TEST_CASE("htmlrenderer: cells are separated by space if `border' is not set") {
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
	REQUIRE(lines[0] == p(LineType::wrappable, "hello world"));
	REQUIRE(links.size() == 0);
}

TEST_CASE("htmlrenderer: cells are separated by vertical bar if `border' is set (regardless of actual value)") {
	htmlrenderer r;
	std::vector<std::pair<LineType, std::string>> lines;
	std::vector<linkpair> links;

	for (auto border_width = 1; border_width < 10; ++border_width) {
		SECTION(utils::strprintf("`border' = %u", border_width)) {
		const std::string input_template =
			"<table border='%u'>"
				"<tr>"
					"<td>hello</td>"
					"<td>world</td>"
				"</tr>"
			"</table>";
		const std::string input =
			utils::strprintf(input_template.c_str(), border_width);

		REQUIRE_NOTHROW(r.render(input, lines, links, url));
		REQUIRE(lines.size() == 3);
		REQUIRE(lines[1] == p(LineType::wrappable, "|hello|world|"));
		REQUIRE(links.size() == 0);
		}
	}
}

TEST_CASE("htmlrenderer: tables with `border' have borders") {
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
	REQUIRE(lines[0] == p(LineType::wrappable, "+-----+-----+"));
	REQUIRE(lines[1] == p(LineType::wrappable, "|hello|world|"));
	REQUIRE(lines[2] == p(LineType::wrappable, "+-----+-----+"));
	REQUIRE(links.size() == 0);
}

TEST_CASE("htmlrenderer: if document ends before </table> is found, table is rendered anyway") {
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
	REQUIRE(lines[0] == p(LineType::wrappable, "+-----+-----+"));
	REQUIRE(lines[1] == p(LineType::wrappable, "|hello|world|"));
	REQUIRE(lines[2] == p(LineType::wrappable, "+-----+-----+"));
	REQUIRE(links.size() == 0);
}

TEST_CASE("htmlrenderer: tables can be nested") {
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
	REQUIRE(lines[0] == p(LineType::wrappable, "+---------------+-----------+"));
	REQUIRE(lines[1] == p(LineType::wrappable, "|+-------+-----+|lonely cell|"));
	REQUIRE(lines[2] == p(LineType::wrappable, "||hello  |world||           |"));
	REQUIRE(lines[3] == p(LineType::wrappable, "|+-------+-----+|           |"));
	REQUIRE(lines[4] == p(LineType::wrappable, "||another|row  ||           |"));
	REQUIRE(lines[5] == p(LineType::wrappable, "|+-------+-----+|           |"));
	REQUIRE(lines[6] == p(LineType::wrappable, "+---------------+-----------+"));

	REQUIRE(links.size() == 0);
}

TEST_CASE("htmlrenderer: if <td> appears inside table but outside of a row, one is created implicitly") {
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
	REQUIRE(lines[0] == p(LineType::wrappable, "+-----+-----+"));
	REQUIRE(lines[1] == p(LineType::wrappable, "|hello|world|"));
	REQUIRE(lines[2] == p(LineType::wrappable, "+-----+-----+"));
	REQUIRE(links.size() == 0);
}

TEST_CASE("htmlrenderer: previous row is implicitly closed when <tr> is found") {
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
	REQUIRE(lines[0] == p(LineType::wrappable, "+-----+"));
	REQUIRE(lines[1] == p(LineType::wrappable, "|hello|"));
	REQUIRE(lines[2] == p(LineType::wrappable, "+-----+"));
	REQUIRE(lines[3] == p(LineType::wrappable, "|world|"));
	REQUIRE(lines[4] == p(LineType::wrappable, "+-----+"));
	REQUIRE(links.size() == 0);
}

TEST_CASE("htmlrenderer: free-standing text outside of <td> is implicitly concatenated with the next cell") {
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
	REQUIRE(lines[0] == p(LineType::wrappable, "+-----------+"));
	REQUIRE(lines[1] == p(LineType::wrappable, "|hello world|"));
	REQUIRE(lines[2] == p(LineType::wrappable, "+-----------+"));
	REQUIRE(links.size() == 0);
}

TEST_CASE("htmlrenderer: text within <ituneshack> is to be treated specially") {
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

TEST_CASE("htmlrenderer: <hr> is rendered correctly") {
	htmlrenderer r;
	std::string result;

	SECTION("width = 3") {
			REQUIRE_NOTHROW(result = r.render_hr(3));
			REQUIRE(result == "\n - \n");
	}

	SECTION("width = 10") {
			REQUIRE_NOTHROW(result = r.render_hr(10));
			REQUIRE(result == "\n -------- \n");
	}

	SECTION("width = 72") {
			REQUIRE_NOTHROW(result = r.render_hr(72));
			REQUIRE(result == "\n ---------------------------------------------------------------------- \n");
	}
}

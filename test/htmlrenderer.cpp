#include "htmlrenderer.h"

#include <sstream>

#include "3rd-party/catch.hpp"
#include "strprintf.h"
#include "utils.h"

using namespace Newsboat;

const std::string url = "http://example.com/feed.rss";

/*
 * We're going to do an awful lot of comparisons of these pairs, so it makes
 * sense to create a function to make constructing them easier.
 *
 * And while we're at it, let's make it accept char* instead of string so that
 * we don't have to wrap string literals in std::string() calls (we don't
 * support C++14 yet so can't use their fancy string literal operators.)
 */
std::pair<Newsboat::LineType, std::string> p(Newsboat::LineType type,
	const char* str)
{
	return std::make_pair(type, std::string(str));
}

namespace Catch {
/* Catch doesn't know how to print out Newsboat::LineType values, so
 * let's teach it!
 *
 * Technically, any one of the following two definitions shouls be enough,
 * but somehow it's not that way: toString works in std::pair<> while
 * StringMaker is required for simple things like REQUIRE(LineType::hr ==
 * LineType::hr). Weird, but at least this works.
 */
std::string toString(Newsboat::LineType const& value)
{
	switch (value) {
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

template<>
struct StringMaker<Newsboat::LineType> {
	static std::string convert(Newsboat::LineType const& value)
	{
		return toString(value);
	}
};

// Catch also doesn't know about std::pair
template<>
struct StringMaker<std::pair<Newsboat::LineType, std::string>> {
	static std::string convert(
		std::pair<Newsboat::LineType, std::string> const& value)
	{
		std::ostringstream o;
		o << "(";
		o << toString(value.first);
		o << ", \"";
		o << value.second;
		o << "\")";
		return o.str();
	}
};
} // namespace Catch

TEST_CASE(
	"Links are rendered as underlined text with reference number in "
	"square brackets",
	"[HtmlRenderer]")
{
	HtmlRenderer rnd;
	std::vector<std::pair<LineType, std::string>> lines;
	Links links;

	rnd.render("<a href=\"http://slashdot.org/\">slashdot</a>",
		lines,
		links,
		"");

	REQUIRE(lines.size() == 1);

	REQUIRE(lines[0] == p(LineType::wrappable, "<u>slashdot</>[1]"));

	REQUIRE(links[0].url == "http://slashdot.org/");
	REQUIRE(links[0].type == LinkType::HREF);
}

TEST_CASE("<br>, <br/> and <br /> result in a line break", "[HtmlRenderer]")
{
	HtmlRenderer rnd;
	std::vector<std::pair<LineType, std::string>> lines;
	Links links;

	for (std::string tag : {
			"<br>", "<br/>", "<br />"
		}) {
		SECTION(tag) {
			auto input = "hello" + tag + "world!";
			rnd.render(input, lines, links, "");
			REQUIRE(lines.size() == 2);
			REQUIRE(lines[0] == p(LineType::wrappable, "hello"));
			REQUIRE(lines[1] == p(LineType::wrappable, "world!"));
		}
	}
}

TEST_CASE("Superscript is rendered with caret symbol", "[HtmlRenderer]")
{
	HtmlRenderer rnd;
	std::vector<std::pair<LineType, std::string>> lines;
	Links links;

	rnd.render("3<sup>10</sup>", lines, links, "");
	REQUIRE(lines.size() == 1);
	REQUIRE(lines[0] == p(LineType::wrappable, "3^10"));
}

TEST_CASE("Subscript is rendered with square brackets", "[HtmlRenderer]")
{
	HtmlRenderer rnd;
	std::vector<std::pair<LineType, std::string>> lines;
	Links links;

	rnd.render("A<sub>i</sub>", lines, links, "");
	REQUIRE(lines.size() == 1);
	REQUIRE(lines[0] == p(LineType::wrappable, "A[i]"));
}

TEST_CASE("Script tags are ignored", "[HtmlRenderer]")
{
	HtmlRenderer rnd;
	std::vector<std::pair<LineType, std::string>> lines;
	Links links;

	rnd.render("abc<script></script>", lines, links, "");
	REQUIRE(lines.size() == 1);
	REQUIRE(lines[0] == p(LineType::wrappable, "abc"));
}

TEST_CASE("format_ol_count formats list count in specified format",
	"[HtmlRenderer]")
{
	HtmlRenderer r;

	SECTION("format to digit") {
		REQUIRE(r.format_ol_count(1, '1') == " 1");
		REQUIRE(r.format_ol_count(3, '1') == " 3");
	}

	SECTION("format to alphabetic") {
		REQUIRE(r.format_ol_count(3, 'a') == "c");
		REQUIRE(r.format_ol_count(26 + 3, 'a') == "ac");
		REQUIRE(r.format_ol_count(3 * 26 * 26 + 5 * 26 + 2, 'a') ==
			"ceb");
	}

	SECTION("format to alphabetic uppercase") {
		REQUIRE(r.format_ol_count(3, 'A') == "C");
		REQUIRE(r.format_ol_count(26 + 5, 'A') == "AE");
		REQUIRE(r.format_ol_count(27, 'A') == "AA");
		REQUIRE(r.format_ol_count(26, 'A') == "Z");
		REQUIRE(r.format_ol_count(26 * 26 + 26, 'A') == "ZZ");
		REQUIRE(r.format_ol_count(25 * 26 * 26 + 26 * 26 + 26, 'A') ==
			"YZZ");
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
	"[HtmlRenderer]")
{
	HtmlRenderer r;

	const std::string input =
		"<a href='http://example.com/about'>About us</a>"
		"<a href='http://example.com/about'>Another link</a>";
	std::vector<std::pair<LineType, std::string>> lines;
	Links links;

	REQUIRE_NOTHROW(r.render(input, lines, links, url));
	REQUIRE(links.size() == 1);
	REQUIRE(links[0].url == "http://example.com/about");
	REQUIRE(links[0].type == LinkType::HREF);
}

TEST_CASE("links with different URLs have different numbers", "[HtmlRenderer]")
{
	HtmlRenderer r;

	const std::string input =
		"<a href='http://example.com/one'>One</a>"
		"<a href='http://example.com/two'>Two</a>";
	std::vector<std::pair<LineType, std::string>> lines;
	Links links;

	REQUIRE_NOTHROW(r.render(input, lines, links, url));
	REQUIRE(links.size() == 2);
	REQUIRE(links[0].url == "http://example.com/one");
	REQUIRE(links[0].type == LinkType::HREF);
	REQUIRE(links[1].url == "http://example.com/two");
	REQUIRE(links[1].type == LinkType::HREF);
}

TEST_CASE("link without `href' is neither highlighted nor added to links list",
	"[HtmlRenderer]")
{
	HtmlRenderer r;

	const std::string input = "<a>test</a>";
	std::vector<std::pair<LineType, std::string>> lines;
	Links links;

	REQUIRE_NOTHROW(r.render(input, lines, links, url));
	REQUIRE(lines.size() == 1);
	REQUIRE(lines[0] == p(LineType::wrappable, "test"));
	REQUIRE(links.size() == 0);
}

TEST_CASE(
	"link with empty `href' is neither highlighted nor added to links list",
	"[HtmlRenderer]")
{
	HtmlRenderer r;

	const std::string input = "<a href=''>test</a>";
	std::vector<std::pair<LineType, std::string>> lines;
	Links links;

	REQUIRE_NOTHROW(r.render(input, lines, links, url));
	REQUIRE(lines.size() == 1);
	REQUIRE(lines[0] == p(LineType::wrappable, "test"));
	REQUIRE(links.size() == 0);
}

TEST_CASE("<strong> is rendered in bold font", "[HtmlRenderer]")
{
	HtmlRenderer r;

	const std::string input = "<strong>test</strong>";
	std::vector<std::pair<LineType, std::string>> lines;
	Links links;

	REQUIRE_NOTHROW(r.render(input, lines, links, url));
	REQUIRE(lines.size() == 1);
	REQUIRE(lines[0] == p(LineType::wrappable, "<b>test</>"));
}

TEST_CASE("<u> is rendered as underlined text", "[HtmlRenderer]")
{
	HtmlRenderer r;

	const std::string input = "<u>test</u>";
	std::vector<std::pair<LineType, std::string>> lines;
	Links links;

	REQUIRE_NOTHROW(r.render(input, lines, links, url));
	REQUIRE(lines.size() == 1);
	REQUIRE(lines[0] == p(LineType::wrappable, "<u>test</>"));
}

TEST_CASE("<q> is rendered as text in quotes", "[HtmlRenderer]")
{
	HtmlRenderer r;

	const std::string input = "<q>test</q>";
	std::vector<std::pair<LineType, std::string>> lines;
	Links links;

	REQUIRE_NOTHROW(r.render(input, lines, links, url));
	REQUIRE(lines.size() == 1);
	REQUIRE(lines[0] == p(LineType::wrappable, "\"test\""));
}

TEST_CASE("Flash <embed>s are added to links if `src' is set", "[HtmlRenderer]")
{
	HtmlRenderer r;

	const std::string input =
		"<embed type='application/x-shockwave-flash'"
		"src='http://example.com/game.swf'>"
		"</embed>";
	std::vector<std::pair<LineType, std::string>> lines;
	Links links;

	REQUIRE_NOTHROW(r.render(input, lines, links, url));
	REQUIRE(lines.size() == 1);
	REQUIRE(lines[0] == p(LineType::wrappable, "[embedded flash: 1]"));

	REQUIRE(links.size() == 1);
	REQUIRE(links[0].url == "http://example.com/game.swf");
	REQUIRE(links[0].type == LinkType::EMBED);
}

TEST_CASE("Flash <embed>s are ignored if `src' is not set", "[HtmlRenderer]")
{
	HtmlRenderer r;

	const std::string input =
		"<embed type='application/x-shockwave-flash'></embed>";
	std::vector<std::pair<LineType, std::string>> lines;
	Links links;

	REQUIRE_NOTHROW(r.render(input, lines, links, url));
	REQUIRE(lines.size() == 0);
	REQUIRE(links.size() == 0);
}

TEST_CASE("non-flash <embed>s are ignored", "[HtmlRenderer]")
{
	HtmlRenderer r;

	const std::string input =
		"<embed type='whatever'"
		"src='http://example.com/thingy'></embed>";
	std::vector<std::pair<LineType, std::string>> lines;
	Links links;

	REQUIRE_NOTHROW(r.render(input, lines, links, url));
	REQUIRE(lines.size() == 0);
	REQUIRE(links.size() == 0);
}

TEST_CASE("<embed>s are ignored if `type' is not set", "[HtmlRenderer]")
{
	HtmlRenderer r;

	const std::string input =
		"<embed src='http://example.com/yet.another.thingy'></embed>";
	std::vector<std::pair<LineType, std::string>> lines;
	Links links;

	REQUIRE_NOTHROW(r.render(input, lines, links, url));
	REQUIRE(lines.size() == 0);
	REQUIRE(links.size() == 0);
}

TEST_CASE("<iframe>s are added to links if `src' is set", "[HtmlRenderer]")
{
	HtmlRenderer r;

	const std::string input(
		"<iframe src=\"https://www.youtube.com/embed/0123456789A\""
		"        width=\"640\" height=\"360\"></iframe>");
	std::vector<std::pair<LineType, std::string>> lines;
	Links links;

	REQUIRE_NOTHROW(r.render(input, lines, links, ""));
	REQUIRE(lines.size() == 1);
	REQUIRE(lines[0] == p(LineType::wrappable, "[iframe 1 (link #1)]"));

	REQUIRE(links.size() == 1);
	REQUIRE(links[0].url == "https://www.youtube.com/embed/0123456789A");
	REQUIRE(links[0].type == LinkType::IFRAME);
}

TEST_CASE("<iframe>s are rendered with a title if `title' is set",
	"[HtmlRenderer]")
{
	HtmlRenderer r;

	const std::string input(
		"<iframe src=\"https://www.youtube.com/embed/0123456789A\""
		"        title=\"My Video\" width=\"640\" height=\"360\"></iframe>");
	std::vector<std::pair<LineType, std::string>> lines;
	Links links;

	REQUIRE_NOTHROW(r.render(input, lines, links, ""));
	REQUIRE(lines.size() == 1);
	REQUIRE(lines[0] == p(LineType::wrappable,
			"[iframe 1: My Video (link #1)]"));

	REQUIRE(links.size() == 1);
	REQUIRE(links[0].url == "https://www.youtube.com/embed/0123456789A");
	REQUIRE(links[0].type == LinkType::IFRAME);
}

TEST_CASE("<iframe>s are ignored if `src' is not set", "[HtmlRenderer]")
{
	HtmlRenderer r;

	const std::string input(
		"<iframe width=\"640\" height=\"360\"></iframe>");
	std::vector<std::pair<LineType, std::string>> lines;
	Links links;

	REQUIRE_NOTHROW(r.render(input, lines, links, ""));
	REQUIRE(lines.size() == 0);
	REQUIRE(links.size() == 0);
}

TEST_CASE("spaces and line breaks are preserved inside <pre>", "[HtmlRenderer]")
{
	HtmlRenderer r;

	const std::string input =
		"<pre>oh cool\n"
		"\n"
		"  check this\tstuff  out!\n"
		"     \n"
		"neat huh?\n"
		"</pre>";
	std::vector<std::pair<LineType, std::string>> lines;
	Links links;

	REQUIRE_NOTHROW(r.render(input, lines, links, url));
	REQUIRE(lines.size() == 5);
	REQUIRE(lines[0] == p(LineType::softwrappable, "oh cool"));
	REQUIRE(lines[1] == p(LineType::softwrappable, ""));
	REQUIRE(lines[2] ==
		p(LineType::softwrappable, "  check this\tstuff  out!"));
	REQUIRE(lines[3] == p(LineType::softwrappable, "     "));
	REQUIRE(lines[4] == p(LineType::softwrappable, "neat huh?"));
	REQUIRE(links.size() == 0);
}

TEST_CASE("tags still work inside <pre>", "[HtmlRenderer]")
{
	HtmlRenderer r;

	const std::string input =
		"<pre>"
		"<strong>bold text</strong>"
		"<u>underlined text</u>"
		"</pre>";
	std::vector<std::pair<LineType, std::string>> lines;
	Links links;

	REQUIRE_NOTHROW(r.render(input, lines, links, url));
	REQUIRE(lines.size() == 1);
	REQUIRE(lines[0] ==
		p(LineType::softwrappable,
			"<b>bold text</><u>underlined text</>"));
	REQUIRE(links.size() == 0);
}

TEST_CASE("line breaks are preserved in tags inside <pre>", "[HtmlRenderer]")
{
	HtmlRenderer r;

	const std::string input =
		"<pre><span>\n\n\n"
		"very cool</span><span>very cool indeed\n\n\n"
		"</span></pre>";
	std::vector<std::pair<LineType, std::string>> lines;
	Links links;

	REQUIRE_NOTHROW(r.render(input, lines, links, url));
	REQUIRE(lines.size() == 4);
	REQUIRE(lines[0] == p(LineType::softwrappable, ""));
	REQUIRE(lines[1] == p(LineType::softwrappable, ""));
	REQUIRE(lines[2] == p(LineType::softwrappable, "very coolvery cool indeed"));
	REQUIRE(lines[3] == p(LineType::softwrappable, ""));
	REQUIRE(links.size() == 0);
}

TEST_CASE("<img> results in a placeholder and a link", "[HtmlRenderer]")
{
	HtmlRenderer r;

	const std::string input =
		"<img src='http://example.com/image.png'></img>";
	std::vector<std::pair<LineType, std::string>> lines;
	Links links;

	REQUIRE_NOTHROW(r.render(input, lines, links, url));
	REQUIRE(lines.size() == 1);
	REQUIRE(lines[0] == p(LineType::wrappable, "[image 1 (link #1)]"));

	REQUIRE(links.size() == 1);
	REQUIRE(links[0].url == "http://example.com/image.png");
	REQUIRE(links[0].type == LinkType::IMG);
}

TEST_CASE(
	"<img> results in a placeholder with the correct index, "
	"and a link",
	"[HtmlRenderer]")
{
	HtmlRenderer r;

	const std::string input =
		"<a href='http://example.com/index.html'>My Page</a>"
		" and an image: "
		"<img src='http://example.com/image.png'></img>";
	std::vector<std::pair<LineType, std::string>> lines;
	Links links;

	REQUIRE_NOTHROW(r.render(input, lines, links, url));
	REQUIRE(lines.size() == 1);
	REQUIRE(lines[0] == p(LineType::wrappable,
			"<u>My Page</>[1] and an image: [image 1 (link #2)]"));

	REQUIRE(links.size() == 2);
	REQUIRE(links[0].url == "http://example.com/index.html");
	REQUIRE(links[0].type == LinkType::HREF);
	REQUIRE(links[1].url == "http://example.com/image.png");
	REQUIRE(links[1].type == LinkType::IMG);
}

TEST_CASE(
	"<img>s without `src', or with empty `src', are ignored",
	"[HtmlRenderer]")
{
	HtmlRenderer r;

	const std::string input =
		"<img></img>"
		"<img src=''></img>"
		"<img src='http://example.com/image.png'>";
	std::vector<std::pair<LineType, std::string>> lines;
	Links links;

	REQUIRE_NOTHROW(r.render(input, lines, links, url));
	REQUIRE(lines.size() == 1);
	REQUIRE(lines[0] == p(LineType::wrappable, "[image 1 (link #1)]"));
	REQUIRE(links.size() == 1);
	REQUIRE(links[0].url == "http://example.com/image.png");
	REQUIRE(links[0].type == LinkType::IMG);
}

TEST_CASE("alt is mentioned in placeholder if <img> has `alt'",
	"[HtmlRenderer]")
{
	HtmlRenderer r;

	const std::string input =
		"<img src='http://example.com/image.png'"
		"alt='Just a test image'></img>";
	std::vector<std::pair<LineType, std::string>> lines;
	Links links;

	REQUIRE_NOTHROW(r.render(input, lines, links, url));
	REQUIRE(lines.size() == 1);
	REQUIRE(lines[0] == p(LineType::wrappable,
			"[image 1: Just a test image (link #1)]"));
	REQUIRE(links.size() == 1);
	REQUIRE(links[0].url == "http://example.com/image.png");
	REQUIRE(links[0].type == LinkType::IMG);
}

TEST_CASE("alt is mentioned in placeholder if <img> has `alt' and `title",
	"[HtmlRenderer]")
{
	HtmlRenderer r;

	const std::string input =
		"<img src='http://example.com/image.png'"
		"alt='Just a test image' title='Image title'></img>";
	std::vector<std::pair<LineType, std::string>> lines;
	Links links;

	REQUIRE_NOTHROW(r.render(input, lines, links, url));
	REQUIRE(lines.size() == 1);
	REQUIRE(lines[0] == p(LineType::wrappable,
			"[image 1: Just a test image (link #1)]"));
	REQUIRE(links.size() == 1);
	REQUIRE(links[0].url == "http://example.com/image.png");
	REQUIRE(links[0].type == LinkType::IMG);
}

TEST_CASE(
	"title is mentioned in placeholder if <img> has `title' but not `alt'",
	"[HtmlRenderer]")
{
	HtmlRenderer r;

	const std::string input =
		"<img src='http://example.com/image.png'"
		"title='Just a test image'></img>";
	std::vector<std::pair<LineType, std::string>> lines;
	Links links;

	REQUIRE_NOTHROW(r.render(input, lines, links, url));
	REQUIRE(lines.size() == 1);
	REQUIRE(lines[0] ==
		p(LineType::wrappable, "[image 1: Just a test image (link #1)]"));
	REQUIRE(links.size() == 1);
	REQUIRE(links[0].url == "http://example.com/image.png");
	REQUIRE(links[0].type == LinkType::IMG);
}

TEST_CASE(
	"URL of <img> with data inside `src' is replaced with string "
	"\"inline image\"",
	"[HtmlRenderer]")
{
	HtmlRenderer r;

	const std::string input =
		"<img src='data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAUA"
		"AAAFCAYAAACNbyblAAAAHElEQVQI12P4//8/w38GIAXDIBKE0DHxgljNBAAO"
		"9TXL0Y4OHwAAAABJRU5ErkJggg==' alt='Red dot' />";
	std::vector<std::pair<LineType, std::string>> lines;
	Links links;

	REQUIRE_NOTHROW(r.render(input, lines, links, url));
	REQUIRE(lines.size() == 1);
	REQUIRE(lines[0] == p(LineType::wrappable, "[image 1: Red dot (link #1)]"));
	REQUIRE(links.size() == 1);
	REQUIRE(links[0].url == "inline image");
	REQUIRE(links[0].type == LinkType::IMG);
}

TEST_CASE(
	"Links in tags like <img> and <iframe> have a \"higher priority\" than <a>",
	"[HtmlRenderer]")
{
	HtmlRenderer r;

	const std::string input =
		"<a href='https://example.com/test.jpg'>"
		"<img src='https://example.com/test.jpg' />"
		"</a>"
		"<p>Check out <a href='https://attachment.zip'>this amazing site</a>!</p>"
		"<iframe src='https://attachment.zip'></iframe>";
	std::vector<std::pair<LineType, std::string>> lines;
	Links links;

	REQUIRE_NOTHROW(r.render(input, lines, links, url));
	REQUIRE(lines.size() == 5);
	REQUIRE(lines[0] == p(LineType::wrappable, "<u>[image 1 (link #1)]</>[1]"));
	REQUIRE(lines[1] == p(LineType::wrappable, ""));
	REQUIRE(lines[2] == p(LineType::wrappable, "Check out <u>this amazing site</>[2]!"));
	REQUIRE(lines[3] == p(LineType::wrappable, ""));
	REQUIRE(lines[4] == p(LineType::wrappable, "[iframe 1 (link #2)]"));
	REQUIRE(links.size() == 2);
	REQUIRE(links[0].url == "https://example.com/test.jpg");
	REQUIRE(links[0].type == LinkType::IMG);
	REQUIRE(links[1].url == "https://attachment.zip/");
	REQUIRE(links[1].type == LinkType::IFRAME);
}

TEST_CASE("<blockquote> is indented and is separated by empty lines",
	"[HtmlRenderer]")
{
	HtmlRenderer r;

	const std::string input =
		"<blockquote>"
		"Experience is what you get when you didn't get what you "
		"wanted. "
		"&mdash;Randy Pausch"
		"</blockquote>";
	std::vector<std::pair<LineType, std::string>> lines;
	Links links;

	REQUIRE_NOTHROW(r.render(input, lines, links, url));
	REQUIRE(lines.size() == 3);
	REQUIRE(lines[0] == p(LineType::wrappable, ""));
	REQUIRE(lines[1] ==
		p(LineType::wrappable,
			"  Experience is what you get when you didn't get "
			"what you wanted. —Randy Pausch"));
	REQUIRE(lines[2] == p(LineType::wrappable, ""));
	REQUIRE(links.size() == 0);
}

TEST_CASE(
	"<dl>, <dt> and <dd> are rendered as a set of paragraphs with term "
	"descriptions indented to the right",
	"[HtmlRenderer]")
{
	HtmlRenderer r;

	const std::string input =
		// Opinions are lifted from the "Monstrous Regiment" by Terry
		// Pratchett
		"<dl>"
		"<dt>Coffee</dt>"
		"<dd>Foul muck</dd>"
		"<dt>Tea</dt>"
		"<dd>Soldier's friend</dd>"
		"</dl>";
	std::vector<std::pair<LineType, std::string>> lines;
	Links links;

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

TEST_CASE("<h[2-6]> and <p>", "[HtmlRenderer]")
{
	HtmlRenderer r;
	std::vector<std::pair<LineType, std::string>> lines;
	Links links;

	SECTION("<h1> is rendered with setext-style underlining") {
		const std::string input = "<h1>Why are we here?</h1>";

		REQUIRE_NOTHROW(r.render(input, lines, links, url));
		REQUIRE(lines.size() == 2);
		REQUIRE(lines[0] == p(LineType::wrappable, "Why are we here?"));
		REQUIRE(lines[1] == p(LineType::wrappable, "----------------"));
		REQUIRE(links.size() == 0);
	}

	for (auto tag : {
			"<h2>", "<h3>", "<h4>", "<h5>", "<h6>", "<p>"
		}) {
		DYNAMIC_SECTION("When alone, " << tag << " generates only one line") {
			std::string closing_tag = tag;
			closing_tag.insert(1, "/");

			const std::string input =
				tag + std::string("hello world") + closing_tag;
			REQUIRE_NOTHROW(r.render(input, lines, links, url));
			REQUIRE(lines.size() == 1);
			REQUIRE(lines[0] ==
				p(LineType::wrappable, "hello world"));
			REQUIRE(links.size() == 0);
		}
	}

	SECTION("There's always an empty line between header/paragraph/list "
		"and paragraph") {
		SECTION("<h1>") {
			const std::string input =
				"<h1>header</h1><p>paragraph</p>";
			REQUIRE_NOTHROW(r.render(input, lines, links, url));
			REQUIRE(lines.size() == 4);
			REQUIRE(lines[2] == p(LineType::wrappable, ""));
			REQUIRE(links.size() == 0);
		}

		for (auto tag : {
				"<h2>", "<h3>", "<h4>", "<h5>", "<h6>", "<p>"
			}) {
			SECTION(tag) {
				std::string closing_tag = tag;
				closing_tag.insert(1, "/");

				const std::string input = tag +
					std::string("header") + closing_tag +
					"<p>paragraph</p>";

				REQUIRE_NOTHROW(
					r.render(input, lines, links, url));
				REQUIRE(lines.size() == 3);
				REQUIRE(lines[1] == p(LineType::wrappable, ""));
				REQUIRE(links.size() == 0);
			}
		}

		SECTION("<ul>") {
			const std::string input =
				"<ul><li>one</li><li>two</li></"
				"ul><p>paragraph</p>";
			REQUIRE_NOTHROW(r.render(input, lines, links, url));
			REQUIRE(lines.size() == 5);
			REQUIRE(lines[3] == p(LineType::wrappable, ""));
			REQUIRE(links.size() == 0);
		}

		SECTION("<ol>") {
			const std::string input =
				"<ol><li>one</li><li>two</li></"
				"ol><p>paragraph</p>";
			REQUIRE_NOTHROW(r.render(input, lines, links, url));
			REQUIRE(lines.size() == 5);
			REQUIRE(lines[3] == p(LineType::wrappable, ""));
			REQUIRE(links.size() == 0);
		}
	}
}

TEST_CASE("whitespace is erased at the beginning of the paragraph",
	"[HtmlRenderer]")
{
	HtmlRenderer r;

	const std::string input =
		"<p>"
		"     \n"
		"here comes the text"
		"</p>";
	std::vector<std::pair<LineType, std::string>> lines;
	Links links;

	REQUIRE_NOTHROW(r.render(input, lines, links, url));
	REQUIRE(lines.size() == 1);
	REQUIRE(lines[0] == p(LineType::wrappable, "here comes the text"));
	REQUIRE(links.size() == 0);
}

TEST_CASE("newlines are replaced with space", "[HtmlRenderer]")
{
	HtmlRenderer r;

	const std::string input =
		"newlines\nshould\nbe\nreplaced\nwith\na\nspace\ncharacter.";
	std::vector<std::pair<LineType, std::string>> lines;
	Links links;

	REQUIRE_NOTHROW(r.render(input, lines, links, url));
	REQUIRE(lines.size() == 1);
	REQUIRE(lines[0] ==
		p(LineType::wrappable,
			"newlines should be replaced with a space character."));
	REQUIRE(links.size() == 0);
}

TEST_CASE("paragraph is just a long line of text", "[HtmlRenderer]")
{
	HtmlRenderer r;

	const std::string input =
		"<p>"
		"here comes a long, boring chunk text that we have to fit to "
		"width"
		"</p>";
	std::vector<std::pair<LineType, std::string>> lines;
	Links links;

	REQUIRE_NOTHROW(r.render(input, lines, links, url));
	REQUIRE(lines.size() == 1);
	REQUIRE(lines[0] ==
		p(LineType::wrappable,
			"here comes a long, boring chunk text "
			"that we have to fit to width"));
	REQUIRE(links.size() == 0);
}

TEST_CASE("default style for <ol> is Arabic numerals", "[HtmlRenderer]")
{
	HtmlRenderer r;
	std::vector<std::pair<LineType, std::string>> lines;
	Links links;

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

TEST_CASE("default starting number for <ol> is 1", "[HtmlRenderer]")
{
	HtmlRenderer r;
	std::vector<std::pair<LineType, std::string>> lines;
	Links links;

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

TEST_CASE("type='1' for <ol> means Arabic numbering", "[HtmlRenderer]")
{
	HtmlRenderer r;
	std::vector<std::pair<LineType, std::string>> lines;
	Links links;
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
	"[HtmlRenderer]")
{
	HtmlRenderer r;
	std::vector<std::pair<LineType, std::string>> lines;
	Links links;
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
	"[HtmlRenderer]")
{
	HtmlRenderer r;
	std::vector<std::pair<LineType, std::string>> lines;
	Links links;
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

TEST_CASE("type='i' for <ol> means lowercase Roman numbering", "[HtmlRenderer]")
{
	HtmlRenderer r;
	std::vector<std::pair<LineType, std::string>> lines;
	Links links;
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

TEST_CASE("type='I' for <ol> means uppercase Roman numbering", "[HtmlRenderer]")
{
	HtmlRenderer r;
	std::vector<std::pair<LineType, std::string>> lines;
	Links links;
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
	"[HtmlRenderer]")
{
	HtmlRenderer r;
	std::vector<std::pair<LineType, std::string>> lines;
	Links links;
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

TEST_CASE("<style> tags are ignored", "[HtmlRenderer]")
{
	HtmlRenderer r;
	std::vector<std::pair<LineType, std::string>> lines;
	Links links;
	const std::string input =
		"<style><h1>ignore me</h1><p>and me</p> body{width: "
		"100%;}</style>";

	REQUIRE_NOTHROW(r.render(input, lines, links, url));
	REQUIRE(lines.size() == 0);
	REQUIRE(links.size() == 0);
}

TEST_CASE("<hr> is not a string, but a special type of line", "[HtmlRenderer]")
{
	std::vector<std::pair<LineType, std::string>> lines;
	Links links;
	HtmlRenderer r;
	const std::string input = "<hr>";

	REQUIRE_NOTHROW(r.render(input, lines, links, url));
	REQUIRE(links.size() == 0);

	REQUIRE(lines.size() == 1);
	REQUIRE(lines[0].first == LineType::hr);
}

TEST_CASE("header rows of tables are in bold", "[HtmlRenderer]")
{
	HtmlRenderer r;
	std::vector<std::pair<LineType, std::string>> lines;
	Links links;

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
		REQUIRE(lines[0] ==
			p(LineType::nonwrappable,
				"<b>another</> <b>header</>"));
		REQUIRE(links.size() == 0);
	}
}

TEST_CASE("cells are separated by space if `border' is not set",
	"[HtmlRenderer]")
{
	HtmlRenderer r;
	std::vector<std::pair<LineType, std::string>> lines;
	Links links;

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

TEST_CASE(
	"cells are separated by vertical bar if `border' is set (regardless "
	"of actual value)",
	"[HtmlRenderer]")
{
	HtmlRenderer r;
	std::vector<std::pair<LineType, std::string>> lines;
	Links links;

	for (auto border_width = 1; border_width < 10; ++border_width) {
		DYNAMIC_SECTION("`border' = " << border_width) {
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
			REQUIRE(lines[1] ==
				p(LineType::nonwrappable, "|hello|world|"));
			REQUIRE(links.size() == 0);
		}
	}
}

TEST_CASE("tables with `border' have borders", "[HtmlRenderer]")
{
	HtmlRenderer r;
	std::vector<std::pair<LineType, std::string>> lines;
	Links links;

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
	"[HtmlRenderer]")
{
	HtmlRenderer r;
	std::vector<std::pair<LineType, std::string>> lines;
	Links links;

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

TEST_CASE("tables can be nested", "[HtmlRenderer]")
{
	HtmlRenderer r;
	std::vector<std::pair<LineType, std::string>> lines;
	Links links;

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
	REQUIRE(lines[0] ==
		p(LineType::nonwrappable, "+---------------+-----------+"));
	REQUIRE(lines[1] ==
		p(LineType::nonwrappable, "|+-------+-----+|lonely cell|"));
	REQUIRE(lines[2] ==
		p(LineType::nonwrappable, "||hello  |world||           |"));
	REQUIRE(lines[3] ==
		p(LineType::nonwrappable, "|+-------+-----+|           |"));
	REQUIRE(lines[4] ==
		p(LineType::nonwrappable, "||another|row  ||           |"));
	REQUIRE(lines[5] ==
		p(LineType::nonwrappable, "|+-------+-----+|           |"));
	REQUIRE(lines[6] ==
		p(LineType::nonwrappable, "+---------------+-----------+"));

	REQUIRE(links.size() == 0);
}

TEST_CASE(
	"if <td> appears inside table but outside of a row, one is created "
	"implicitly",
	"[HtmlRenderer]")
{
	HtmlRenderer r;
	std::vector<std::pair<LineType, std::string>> lines;
	Links links;

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
	"[HtmlRenderer]")
{
	HtmlRenderer r;
	std::vector<std::pair<LineType, std::string>> lines;
	Links links;

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

TEST_CASE(
	"free-standing text outside of <td> is implicitly concatenated with "
	"the next cell",
	"[HtmlRenderer]")
{
	HtmlRenderer r;
	std::vector<std::pair<LineType, std::string>> lines;
	Links links;

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
	"[HtmlRenderer]")
{
	HtmlRenderer r;
	std::vector<std::pair<LineType, std::string>> lines;
	Links links;

	const std::string input =
		"<ituneshack>"
		"hello world!\n"
		"I'm a description from an iTunes feed. "
		"Apple just puts plain text into &lt;summary&gt; tag."
		"</ituneshack>";

	REQUIRE_NOTHROW(r.render(input, lines, links, url));
	REQUIRE(lines.size() == 2);
	REQUIRE(lines[0] == p(LineType::wrappable, "hello world!"));
	REQUIRE(lines[1] ==
		p(LineType::wrappable,
			"I'm a description from an iTunes feed. Apple just "
			"puts plain text into <>summary> tag."));
	REQUIRE(links.size() == 0);
}

TEST_CASE("When rendeing text, HtmlRenderer strips leading whitespace",
	"[HtmlRenderer][issue204]")
{
	HtmlRenderer r;
	std::vector<std::pair<LineType, std::string>> lines;
	Links links;

	const std::string input =
		"		<br />\n"
		"		\n" // tabs
		"       \n"         // spaces
		"		\n" // tabs
		"       \n"         // spaces
		"		\n" // tabs
		"		\n" // tabs
		"		Text preceded by whitespace.";

	REQUIRE_NOTHROW(r.render(input, lines, links, url));
	REQUIRE(lines.size() == 2);
	REQUIRE(lines[0] == p(LineType::wrappable, ""));
	REQUIRE(lines[1] ==
		p(LineType::wrappable, "Text preceded by whitespace."));
	REQUIRE(links.size() == 0);
}

TEST_CASE("<video> results in a placeholder and a link for each valid source",
	"[HtmlRenderer]")
{
	HtmlRenderer r;

	const std::string input =
		"<video src='http://example.com/video.avi'></video>"
		"<video>"
		"	<source src='http://example.com/video2.avi'>"
		"	<source src='http://example.com/video2.mkv'>"
		"</video>";
	std::vector<std::pair<LineType, std::string>> lines;
	Links links;

	REQUIRE_NOTHROW(r.render(input, lines, links, url));
	REQUIRE(lines.size() == 3);
	REQUIRE(lines[0] == p(LineType::wrappable, "[video 1 (link #1)]"));
	REQUIRE(lines[1] == p(LineType::wrappable, "[video 2 (link #2)]"));
	REQUIRE(lines[2] == p(LineType::wrappable, "[video 2 (link #3)]"));
	REQUIRE(links.size() == 3);
	REQUIRE(links[0].url == "http://example.com/video.avi");
	REQUIRE(links[0].type == LinkType::VIDEO);
	REQUIRE(links[1].url == "http://example.com/video2.avi");
	REQUIRE(links[1].type == LinkType::VIDEO);
	REQUIRE(links[2].url == "http://example.com/video2.mkv");
	REQUIRE(links[2].type == LinkType::VIDEO);
}

TEST_CASE("<video>s without valid sources are ignored", "[HtmlRenderer]")
{
	HtmlRenderer r;

	const std::string input = "<video></video>"
		"<video><source><source></video>"
		"<video src=''></video>"
		"<video src='http://example.com/video.avi'></video>";
	std::vector<std::pair<LineType, std::string>> lines;
	Links links;

	REQUIRE_NOTHROW(r.render(input, lines, links, url));
	REQUIRE(lines.size() == 1);
	REQUIRE(lines[0] == p(LineType::wrappable, "[video 1 (link #1)]"));
	REQUIRE(links.size() == 1);
	REQUIRE(links[0].url == "http://example.com/video.avi");
	REQUIRE(links[0].type == LinkType::VIDEO);
}

TEST_CASE("<audio> results in a placeholder and a link for each valid source",
	"[HtmlRenderer]")
{
	HtmlRenderer r;

	const std::string input =
		"<audio src='http://example.com/audio.oga'></audio>"
		"<audio>"
		"	<source src='http://example.com/audio2.mp3'>"
		"	<source src='http://example.com/audio2.m4a'>"
		"</audio>";
	std::vector<std::pair<LineType, std::string>> lines;
	Links links;

	REQUIRE_NOTHROW(r.render(input, lines, links, url));
	REQUIRE(lines.size() == 3);
	REQUIRE(lines[0] == p(LineType::wrappable, "[audio 1 (link #1)]"));
	REQUIRE(lines[1] == p(LineType::wrappable, "[audio 2 (link #2)]"));
	REQUIRE(lines[2] == p(LineType::wrappable, "[audio 2 (link #3)]"));
	REQUIRE(links.size() == 3);
	REQUIRE(links[0].url == "http://example.com/audio.oga");
	REQUIRE(links[0].type == LinkType::AUDIO);
	REQUIRE(links[1].url == "http://example.com/audio2.mp3");
	REQUIRE(links[1].type == LinkType::AUDIO);
	REQUIRE(links[2].url == "http://example.com/audio2.m4a");
	REQUIRE(links[2].type == LinkType::AUDIO);
}

TEST_CASE("<audio>s without valid sources are ignored", "[HtmlRenderer]")
{
	HtmlRenderer r;

	const std::string input = "<audio></audio>"
		"<audio><source><source></audio>"
		"<audio src=''></audio>"
		"<audio src='http://example.com/audio.oga'></audio>";
	std::vector<std::pair<LineType, std::string>> lines;
	Links links;

	REQUIRE_NOTHROW(r.render(input, lines, links, url));
	REQUIRE(lines.size() == 1);
	REQUIRE(lines[0] == p(LineType::wrappable, "[audio 1 (link #1)]"));
	REQUIRE(links.size() == 1);
	REQUIRE(links[0].url == "http://example.com/audio.oga");
	REQUIRE(links[0].type == LinkType::AUDIO);
}

TEST_CASE("Unclosed <video> and <audio> tags are closed upon encounter with a "
	"new media element", "[HtmlRenderer]")
{
	HtmlRenderer r;

	const std::string input =
		"<video src='http://example.com/video.avi'>"
		"	This is fallback text for `the video` element"
		"<video>"
		"	<source src='http://example.com/video2.avi'>"
		"This maybe isn't fallback text, but the spec says that"
		" anything before the closing tag is transparent content"
		"<audio>"
		"	<source src='http://example.com/audio.oga'>"
		"	<source src='http://example.com/audio.m4a'>"
		"	This text should also be interpreted as fallback"
		"<audio src='http://example.com/audio2.mp3'>"
		"	This is additional fallback text"
		"<audio></audio>"
		"Here comes the text!";

	std::vector<std::pair<LineType, std::string>> lines;
	Links links;

	REQUIRE_NOTHROW(r.render(input, lines, links, url));
	REQUIRE(lines.size() == 6);
	REQUIRE(lines[0] == p(LineType::wrappable, "[video 1 (link #1)]"));
	REQUIRE(lines[1] == p(LineType::wrappable, "[video 2 (link #2)]"));
	REQUIRE(lines[2] == p(LineType::wrappable, "[audio 1 (link #3)]"));
	REQUIRE(lines[3] == p(LineType::wrappable, "[audio 1 (link #4)]"));
	REQUIRE(lines[4] == p(LineType::wrappable, "[audio 2 (link #5)]"));
	REQUIRE(lines[5] == p(LineType::wrappable, "Here comes the text!"));
	REQUIRE(links.size() == 5);
	REQUIRE(links[0].url == "http://example.com/video.avi");
	REQUIRE(links[0].type == LinkType::VIDEO);
	REQUIRE(links[1].url == "http://example.com/video2.avi");
	REQUIRE(links[1].type == LinkType::VIDEO);
	REQUIRE(links[2].url == "http://example.com/audio.oga");
	REQUIRE(links[2].type == LinkType::AUDIO);
	REQUIRE(links[3].url == "http://example.com/audio.m4a");
	REQUIRE(links[3].type == LinkType::AUDIO);
	REQUIRE(links[4].url == "http://example.com/audio2.mp3");
	REQUIRE(links[4].type == LinkType::AUDIO);
}

TEST_CASE("Empty <source> tags do not increase the link count. Media elements"
	"without valid sources do not increase the element count",
	"[HtmlRenderer]")
{
	HtmlRenderer r;

	const std::string input =
		"<video></video>"
		"<video>"
		"	<source src='http://example.com/video.avi'>"
		"	<source>"
		"	<source src='http://example.com/video.mkv'>"
		"</video>"
		"<audio></audio>"
		"<audio>"
		"	<source src='http://example.com/audio.mp3'>"
		"	<source>"
		"	<source src='http://example.com/audio.oga'>"
		"</audio>";

	std::vector<std::pair<LineType, std::string>> lines;
	Links links;

	REQUIRE_NOTHROW(r.render(input, lines, links, url));
	REQUIRE(lines.size() == 4);
	REQUIRE(lines[0] == p(LineType::wrappable, "[video 1 (link #1)]"));
	REQUIRE(lines[1] == p(LineType::wrappable, "[video 1 (link #2)]"));
	REQUIRE(lines[2] == p(LineType::wrappable, "[audio 1 (link #3)]"));
	REQUIRE(lines[3] == p(LineType::wrappable, "[audio 1 (link #4)]"));
	REQUIRE(links.size() == 4);
	REQUIRE(links[0].url == "http://example.com/video.avi");
	REQUIRE(links[0].type == LinkType::VIDEO);
	REQUIRE(links[1].url == "http://example.com/video.mkv");
	REQUIRE(links[1].type == LinkType::VIDEO);
	REQUIRE(links[2].url == "http://example.com/audio.mp3");
	REQUIRE(links[2].type == LinkType::AUDIO);
	REQUIRE(links[3].url == "http://example.com/audio.oga");
	REQUIRE(links[3].type == LinkType::AUDIO);
}

TEST_CASE("Back-to-back <video> and <audio> tags are seperated by a new line",
	"[HtmlRenderer]")
{
	HtmlRenderer r;

	const std::string input =
		"<video src='https://example.com/video.mp4'></video>"
		"<audio src='https://example.com/audio.mp3'></audio>";

	std::vector<std::pair<LineType, std::string>> lines;
	Links links;

	REQUIRE_NOTHROW(r.render(input, lines, links, url));
	REQUIRE(lines.size() == 2);
	REQUIRE(lines[0] == p(LineType::wrappable, "[video 1 (link #1)]"));
	REQUIRE(lines[1] == p(LineType::wrappable, "[audio 1 (link #2)]"));
	REQUIRE(links.size() == 2);
	REQUIRE(links[0].url == "https://example.com/video.mp4");
	REQUIRE(links[0].type == LinkType::VIDEO);
	REQUIRE(links[1].url == "https://example.com/audio.mp3");
	REQUIRE(links[1].type == LinkType::AUDIO);
}

TEST_CASE("Ordered list can contain unordered list in its items",
	"[HtmlRenderer]")
{
	HtmlRenderer rnd;
	std::vector<std::pair<LineType, std::string>> lines;
	Links links;

	const auto input = std::string(
			"<ol>"
			"	<li>"
			"		<ul>"
			"			<li>first item of sublist A</li>"
			"			<li>second item of sublist A</li>"
			"		</ul>"
			"	</li>"
			"	<li>"
			"		<ul>"
			"			<li>first item of sublist B</li>"
			"			<li>second item of sublist B</li>"
			"			<li>third item of sublist B</li>"
			"		</ul>"
			"	</li>"
			"</ol>");
	rnd.render(input, lines, links, "");

	REQUIRE(lines.size() == 13);
	REQUIRE(lines[0] == p(LineType::wrappable, ""));
	REQUIRE(lines[1] == p(LineType::wrappable, " 1.  "));
	REQUIRE(lines[2] == p(LineType::wrappable, ""));
	REQUIRE(lines[3] == p(LineType::wrappable, "      * first item of sublist A"));
	REQUIRE(lines[4] == p(LineType::wrappable, "      * second item of sublist A"));
	REQUIRE(lines[5] == p(LineType::wrappable, ""));
	REQUIRE(lines[6] == p(LineType::wrappable, " 2.  "));
	REQUIRE(lines[7] == p(LineType::wrappable, ""));
	REQUIRE(lines[8] == p(LineType::wrappable, "      * first item of sublist B"));
	REQUIRE(lines[9] == p(LineType::wrappable, "      * second item of sublist B"));
	REQUIRE(lines[10] == p(LineType::wrappable, "      * third item of sublist B"));
	REQUIRE(lines[11] == p(LineType::wrappable, ""));
	REQUIRE(lines[12] == p(LineType::wrappable, ""));

	REQUIRE(links.size() == 0);
}

TEST_CASE("Skips contents of <script> tags", "[HtmlRenderer]")
{
	// This is a regression test for https://github.com/Newsboat/Newsboat/issues/1300

	HtmlRenderer rnd;
	std::vector<std::pair<LineType, std::string>> lines;
	Links links;

	const auto input = std::string(
			"<script>"
			"</ul>"
			"</ul>"
			"</ul>"
			"</ul>"
			"</ul>"
			"</ul>"
			"</script>"
			""
			"<p>Visible text</p>");

	rnd.render(input, lines, links, "");

	REQUIRE(lines.size() == 1);
	REQUIRE(links.size() == 0);

	REQUIRE(lines[0].second == "Visible text");
}

TEST_CASE("<div> is always rendered on a new line", "[HtmlRenderer]")
{
	HtmlRenderer rnd;
	std::vector<std::pair<LineType, std::string>> lines;
	Links links;

	const std::string
	input("<div>oh</div>"
		"<div>hello there,</div>"
		"<p>world</p>"
		"<div>!</div>"
		"<div>"
		"    <div>"
		"        <p>hehe</p>"
		"    </div>"
		"</div>");

	rnd.render(input, lines, links, "");

	REQUIRE(lines.size() == 9);
	REQUIRE(lines[0] == p(LineType::wrappable, "oh"));
	REQUIRE(lines[1] == p(LineType::wrappable, ""));
	REQUIRE(lines[2] == p(LineType::wrappable, "hello there,"));
	REQUIRE(lines[3] == p(LineType::wrappable, ""));
	REQUIRE(lines[4] == p(LineType::wrappable, "world"));
	REQUIRE(lines[5] == p(LineType::wrappable, ""));
	REQUIRE(lines[6] == p(LineType::wrappable, "!"));
	REQUIRE(lines[7] == p(LineType::wrappable, ""));
	REQUIRE(lines[8] == p(LineType::wrappable, "hehe"));
	REQUIRE(links.size() == 0);
}

TEST_CASE("HtmlRenderer does not crash on extra closing OL/UL tags", "[HtmlRenderer]")
{
	// This is a regression test for https://github.com/Newsboat/Newsboat/issues/1974

	HtmlRenderer rnd;
	std::vector<std::pair<LineType, std::string>> lines;
	Links links;

	const std::string input =
		"<ul><li>Double closed list</li></ul></ul><ol><li>Other double closed list</li></ol></ol><ul><li>Test</li></ul>";

	rnd.render(input, lines, links, "");
}

/* test driver for newsbeuter */

#include "lemon.h"

#include <climits>
#include <vector>
#include <string>
#include <sstream>

#include <unistd.h>

#include <logger.h>
#include <cache.h>
#include <rss.h>
#include <rss_parser.h>
#include <configcontainer.h>
#include <keymap.h>
#include <tagsouppullparser.h>
#include <urlreader.h>
#include <utils.h>
#include <matcher.h>
#include <history.h>
#include <formatstring.h>
#include <exceptions.h>
#include <regexmanager.h>
#include <htmlrenderer.h>

#include <stdlib.h>

using namespace newsbeuter;

namespace test {

lemon::test<> lemon(0);

void TestNewsbeuterReload() {
	configcontainer * cfg = new configcontainer();
	cache * rsscache = new cache("test-cache.db", cfg);

	rss_parser parser("http://testbed.newsbeuter.org/unit-test/rss.xml", rsscache, cfg, NULL);
	std::tr1::shared_ptr<rss_feed> feed = parser.parse();
	lemon.is(feed->items().size(), 8u, "rss.xml contains 8 items");

	rsscache->externalize_rssfeed(feed, false);
	rsscache->internalize_rssfeed(feed, NULL);
	lemon.is(feed->items().size(), 8u, "feed contains 8 items after externalization/internalization");

	lemon.is(feed->items()[0]->title(), "Teh Saxxi", "first item title");
	lemon.is(feed->items()[7]->title(), "Handy als IR-Detektor", "last item title");

	feed->items()[0]->set_title("Another Title");
	feed->items()[0]->set_pubDate(time(NULL));
	lemon.is(feed->items()[0]->title(), "Another Title", "first item title after set_title");

	rsscache->externalize_rssfeed(feed, false);

	std::tr1::shared_ptr<rss_feed> feed2(new rss_feed(rsscache));
	feed2->set_rssurl("http://testbed.newsbeuter.org/unit-test/rss.xml");
	rsscache->internalize_rssfeed(feed2, NULL);

	lemon.is(feed2->items().size(), 8u, "feed2 contains 8 items after internalizaton");
	lemon.is(feed2->items()[0]->title(), "Another Title", "feed2 first item title");
	lemon.is(feed2->items()[7]->title(), "Handy als IR-Detektor", "feed2 last item title");

	std::vector<std::string> feedurls = rsscache->get_feed_urls();
	lemon.is(feedurls.size(), 1u, "1 feed url");
	lemon.is(feedurls[0], "http://testbed.newsbeuter.org/unit-test/rss.xml", "first feed url");

	std::vector<std::tr1::shared_ptr<rss_feed> > feedv;
	feedv.push_back(feed);

	cfg->set_configvalue("cleanup-on-quit", "true");
	rsscache->cleanup_cache(feedv);

	delete rsscache;
	delete cfg;

	::unlink("test-cache.db");
}

void TestConfigParserContainerAndKeymap() {
	configcontainer * cfg = new configcontainer();
	configparser cfgparser;
	cfg->register_commands(cfgparser);
	keymap k(KM_NEWSBEUTER);
	cfgparser.register_handler("macro", &k);

	try {
		cfgparser.parse("test-config.txt");
	}  catch (const configexception& ex) {
		LOG(LOG_ERROR,"an exception occured while parsing the configuration file: %s", ex.what());
		lemon.fail("TODO: description");
	} catch (...) {
		lemon.fail("TODO: description");
	}

	// test boolean config values
	lemon.is(cfg->get_configvalue("show-read-feeds"), "no", "show-read-feeds is no");
	lemon.is(cfg->get_configvalue_as_bool("show-read-feeds"), false, "show-read-feeds as boolean is false");

	// test string config values
	lemon.is(cfg->get_configvalue("browser"), "firefox", "browser is firefox");

	// test integer config values
	lemon.is(cfg->get_configvalue("max-items"), "100", "max-items is 100");
	lemon.is(cfg->get_configvalue_as_int("max-items"), 100, "max-items as integer is 100");

	// test ~/ expansion for path config values
	std::string cachefilecomp = ::getenv("HOME");
	cachefilecomp.append("/foo");
	lemon.ok(cfg->get_configvalue("cache-file") == cachefilecomp, "cache-file is ~/foo (~ substitution test)");

	delete cfg;

	lemon.is(k.get_operation("ENTER", "feedlist"), OP_OPEN, "ENTER in feedlist -> OP_OPEN");
	lemon.is(k.get_operation("u", "article"), OP_SHOWURLS, "u in article -> OP_SHOWURLS");
	lemon.is(k.get_operation("X", "feedlist"), OP_NIL, "X in feedlist -> OP_NIL");
	lemon.is(k.get_operation("", "feedlist"), OP_NIL, "no key in feedlist -> OP_NIL");

	k.unset_key("ENTER", "all");
	lemon.is(k.get_operation("ENTER", "all"), OP_NIL, "ENTER in all -> OP_NIL after unset_key");
	k.set_key(OP_OPEN, "ENTER", "all");
	lemon.is(k.get_operation("ENTER", "all"), OP_OPEN, "ENTER in all -> OP_OPEN after set_key");

	lemon.is(k.get_opcode("open"), OP_OPEN, "open -> OP_OPEN");
	lemon.is(k.get_opcode("some-noexistent-operation"), OP_NIL, "some-noexistent-operation -> OP_NIL");

	lemon.is(k.getkey(OP_OPEN, "all"), "ENTER", "OP_OPEN in all -> ENTER");
	lemon.is(k.getkey(OP_TOGGLEITEMREAD, "all"), "N", "OP_TOGGLEITEMREAD in all -> N");
	lemon.is(k.getkey(static_cast<operation>(30000), "all"), "<none>", "operation 30000 in all -> <none>");

	lemon.is(k.get_key(" "), ' ', "space -> space (character)");
	lemon.is(k.get_key("U"), 'U', "U -> U (character)");
	lemon.is(k.get_key("~"), '~', "~ -> ~ (character)");
	lemon.is(k.get_key("INVALID"), 0, "INVALID -> 0");
	lemon.is(k.get_key("ENTER"), '\n', "ENTER -> \\n");
	lemon.is(k.get_key("ESC"), '\033', "ESC -> \\033");
	lemon.is(k.get_key("^A"), '\001', "^A -> \\001");

	std::vector<std::string> params;
	try {
		k.handle_action("bind-key", params);
		lemon.fail("bind-key with no parameters threw no exception");
	} catch (const confighandlerexception& e) {
		lemon.ok(std::string(e.what()).length() > 0, "bind-key with no parameters");
	}
	try {
		k.handle_action("unbind-key", params);
		lemon.fail("unbind-key with no parameters threw no exception");
	} catch (const confighandlerexception& e) {
		lemon.ok(std::string(e.what()).length() > 0, "unbind-key with no parameters");
	}
	try {
		k.handle_action("macro", params);
		lemon.fail("macro with no parameters threw no exception");
	} catch (const confighandlerexception& e) {
		lemon.ok(std::string(e.what()).length() > 0, "macro with no parameters");
	}
	params.push_back("r");
	try {
		k.handle_action("bind-key", params);
		lemon.fail("bind-key with only one parameter threw no exception");
	} catch (const confighandlerexception& e) {
		lemon.ok(std::string(e.what()).length() > 0, "bind-key with one parameter");
	}
	try {
		k.handle_action("unbind-key", params);
		lemon.pass("unbind-key with one parameter threw no exception");
	} catch (const confighandlerexception& e) {
		lemon.fail("unbind-key with one parameter threw exception");
	}
	params.push_back("open");
	try {
		k.handle_action("bind-key", params);
		lemon.pass("bind-key with two parameters threw no exception");
	} catch (const confighandlerexception& e) {
		lemon.fail("bind-key with two parameters threw exception");
	}
	try {
		k.handle_action("an-invalid-action", params);
		lemon.fail("an-invalid-action threw no exception");
	} catch (const confighandlerexception& e) {
		lemon.pass("an-invalid-action threw exception");
	}
}

void TestTagSoupPullParser() {
	std::istringstream is("<test><foo quux='asdf' bar=\"qqq\">text</foo>more text<more>&quot;&#33;&#x40;</more><xxx foo=bar baz=\"qu ux\" hi='ho ho ho'></xxx></test>");
	tagsouppullparser xpp;
	tagsouppullparser::event e;
	xpp.setInput(is);

	e = xpp.getEventType();
	lemon.is(e, tagsouppullparser::START_DOCUMENT, "document start");
	e = xpp.next();
	lemon.is(e, tagsouppullparser::START_TAG, "type is start tag");
	lemon.is(xpp.getText(), "test", "tag is test");
	e = xpp.next();
	lemon.is(e, tagsouppullparser::START_TAG, "type is start tag");
	lemon.is(xpp.getText(), "foo", "tag is foo");
	lemon.is(xpp.getAttributeValue("quux"), "asdf", "attribute quux is asdf");
	lemon.is(xpp.getAttributeValue("bar"), "qqq", "attribute bar is qqq");
	e = xpp.next();
	lemon.is(e, tagsouppullparser::TEXT, "type is text");
	lemon.is(xpp.getText(), "text", "text is text");
	e = xpp.next();
	lemon.is(e, tagsouppullparser::END_TAG, "type is end tag");
	lemon.is(xpp.getText(), "foo", "tag is foo");
	e = xpp.next();
	lemon.is(e, tagsouppullparser::TEXT, "type is text");
	lemon.is(xpp.getText(), "more text", "text is more text");
	e = xpp.next();
	lemon.is(e, tagsouppullparser::START_TAG, "type is start tag");
	lemon.is(xpp.getText(), "more", "tag is more");
	e = xpp.next();
	lemon.is(e, tagsouppullparser::TEXT, "type is text");
	lemon.is(xpp.getText(), "\"!@", "text is \"!@");
	e = xpp.next();
	lemon.is(e, tagsouppullparser::END_TAG, "type is end tag");
	lemon.is(xpp.getText(), "more", "tag is more");
	e = xpp.next();
	lemon.is(e, tagsouppullparser::START_TAG, "type is start tag");
	lemon.is(xpp.getText(), "xxx", "tag is xxx");
	lemon.is(xpp.getAttributeValue("foo"), "bar", "attribute foo is bar");
	lemon.is(xpp.getAttributeValue("baz"), "qu ux", "attribute baz is qu ux");
	lemon.is(xpp.getAttributeValue("hi"), "ho ho ho", "attribute hi is ho ho ho");
	e = xpp.next();
	lemon.is(e, tagsouppullparser::END_TAG, "type is end tag");
	lemon.is(xpp.getText(), "xxx", "tag is xxx");
	e = xpp.next();
	lemon.is(e, tagsouppullparser::END_TAG, "type is end tag");
	lemon.is(xpp.getText(), "test", "tag is test");
	e = xpp.next();
	lemon.is(e, tagsouppullparser::END_DOCUMENT, "type is end document");
	e = xpp.next();
	lemon.is(e, tagsouppullparser::END_DOCUMENT, "type is end document (2nd try)");
}

void TestUrlReader() {
	file_urlreader u;
	u.load_config("test-urls.txt");
	lemon.is(u.get_urls().size(), 3u, "test-urls.txt contains 3 URL");
	lemon.is(u.get_urls()[0], "http://test1.url.cc/feed.xml", "test-urls.txt URL 1");
	lemon.is(u.get_urls()[1], "http://anotherfeed.com/", "test-urls.txt URL 2");
	lemon.is(u.get_urls()[2], "http://onemorefeed.at/feed/", "test-urls.txt URL 3");

	lemon.is(u.get_tags("http://test1.url.cc/feed.xml").size(), 2u, "feed 1 has 2 tags");
	lemon.is(u.get_tags("http://test1.url.cc/feed.xml")[0], "tag1", "feed 1 tag 1 is tag1");
	lemon.is(u.get_tags("http://test1.url.cc/feed.xml")[1], "tag2", "feed 1 tag 2 is tag2");
	lemon.is(u.get_tags("http://anotherfeed.com/").size(), 0u, "feed 2 has no tags");
	lemon.is(u.get_tags("http://onemorefeed.at/feed/").size(), 2u, "feed 3 has two tags");

	lemon.is(u.get_alltags().size(), 3u, "3 tags in total");
}

void TestTokenizers() {
	std::vector<std::string> tokens;

	tokens = utils::tokenize("as df qqq");
	lemon.is(tokens.size(), 3u, "tokenize results in 3 tokens");
	lemon.ok(tokens[0] == "as" && tokens[1] == "df" && tokens[2] == "qqq", "tokenize of as df qqq");

	tokens = utils::tokenize(" aa ");
	lemon.is(tokens.size(), 1u, "tokenize of ' aa ' resulted in one token");
	lemon.is(tokens[0], "aa", "first token is aa");

	tokens = utils::tokenize("	");
	lemon.is(tokens.size(), 0u, "tokenize of tab resulted in 0 tokens");
	
	tokens = utils::tokenize("");
	lemon.is(tokens.size(), 0u, "tokenize of empty string resulted in 0 tokens");

	tokens = utils::tokenize_spaced("a b");
	lemon.is(tokens.size(), 3u, "tokenize_spaced of a b resulted in 3 tokens");
	lemon.is(tokens[1], " ", "middle token is space");

	tokens = utils::tokenize_spaced(" a\t b ");
	lemon.is(tokens.size(), 5u, "tokenize_spaced resulted in 5 tokens");
	lemon.is(tokens[0], " ", "first token is space");
	lemon.is(tokens[1], "a", "second token is a");
	lemon.is(tokens[2], " ", "third token is space");

	tokens = utils::tokenize_quoted("asdf \"foobar bla\" \"foo\\r\\n\\tbar\"");
	lemon.is(tokens.size(), 3u, "tokenize_quoted resulted in 3 tokens");
	lemon.is(tokens[0], "asdf", "first token is asdf");
	lemon.is(tokens[1], "foobar bla", "second token is foobar bla");
	lemon.is(tokens[2], "foo\r\n\tbar", "third token contains \\r\\n\\t");

	tokens = utils::tokenize_quoted("  \"foo \\\\xxx\"\t\r \" \"");
	lemon.is(tokens.size(), 2u, "tokenize_quoted resulted in 2 tokens");
	lemon.is(tokens[0], "foo \\xxx", "first token contains \\");
	lemon.is(tokens[1], " ", "second token is space");

	tokens = utils::tokenize_quoted("\"\\\\");
	lemon.is(tokens.size(), 1u, "tokenize_quoted with unbalanced quotes resulted in 1 token");
	lemon.is(tokens[0], "\\", "first token is \\");

	// the following test cases specifically demonstrate a problem of the tokenize_quoted with several \\ sequences directly appended
	tokens = utils::tokenize_quoted("\"\\\\\\\\");
	lemon.is(tokens.size(), 1u, "tokenize_quoted escape test 1 resulted in 1 token");
	lemon.is(tokens[0], "\\\\", "tokenize_quoted escape test 1 token is \\\\");

	tokens = utils::tokenize_quoted("\"\\\\\\\\\\\\");
	lemon.is(tokens.size(), 1u, "tokenize_quoted escape test 2 resulted in 1 token");
	lemon.is(tokens[0], "\\\\\\", "tokenize_quoted escape test 2 token is \\\\\\");

	tokens = utils::tokenize_quoted("\"\\\\\\\\\"");
	lemon.is(tokens.size(), 1u, "tokenize_quoted escape test 3 resulted in 1 token");
	lemon.is(tokens[0], "\\\\", "tokenize_quoted escape test 3 token is \\\\");

	tokens = utils::tokenize_quoted("\"\\\\\\\\\\\\\"");
	lemon.is(tokens.size(), 1u, "tokenize_quoted escape test 4 resulted in 1 token");
	lemon.is(tokens[0], "\\\\\\", "tokenize_quoted escape test 4 token is \\\\\\");
}

struct testmatchable : public matchable {
	virtual bool has_attribute(const std::string& attribname) {
		if (attribname == "abcd" || attribname == "AAAA" || attribname == "tags")
			return true;
		return false;
	}

	virtual std::string get_attribute(const std::string& attribname) {
		if (attribname == "abcd")
			return "xyz";
		if (attribname == "AAAA")
			return "12345";
		if (attribname == "tags")
			return "foo bar baz quux";
		return "";
	}
};

void TestIssue94Crash() {
	FilterParser fp;

	lemon.not_ok(fp.parse_string("title =¯ \"foo\""), "parse expression with invalid character in operator - must not crash.");
}

void TestFilterLanguage() {

	FilterParser fp;

	// test parser
	lemon.ok(fp.parse_string("a = \"b\""), utils::wstr2str(fp.get_error()).c_str());
	lemon.ok(!fp.parse_string("a = \"b"), utils::wstr2str(fp.get_error()).c_str());
	lemon.ok(!fp.parse_string("a = b"), utils::wstr2str(fp.get_error()).c_str());
	lemon.ok(fp.parse_string("(a=\"b\")"), utils::wstr2str(fp.get_error()).c_str());
	lemon.ok(fp.parse_string("((a=\"b\"))"), utils::wstr2str(fp.get_error()).c_str());
	lemon.ok(!fp.parse_string("((a=\"b\")))"), utils::wstr2str(fp.get_error()).c_str());

	// test operators
	lemon.ok(fp.parse_string("a != \"b\""), utils::wstr2str(fp.get_error()).c_str());
	lemon.ok(fp.parse_string("a =~ \"b\""), utils::wstr2str(fp.get_error()).c_str());
	lemon.ok(fp.parse_string("a !~ \"b\""), utils::wstr2str(fp.get_error()).c_str());
	lemon.ok(!fp.parse_string("a !! \"b\""), utils::wstr2str(fp.get_error()).c_str());

	// complex query
	lemon.is(fp.parse_string("( a = \"b\") and ( b = \"c\" ) or ( ( c != \"d\" ) and ( c !~ \"asdf\" )) or c != \"xx\""), true, "parse string of complex query");

	testmatchable tm;
	matcher m;

	m.parse("abcd = \"xyz\"");
	lemon.ok(m.matches(&tm), "abcd = xyz matches");
	m.parse("abcd = \"uiop\"");
	lemon.not_ok(m.matches(&tm), "abcd = uiop doesn't match");
	m.parse("abcd != \"uiop\"");
	lemon.ok(m.matches(&tm), "abcd != uiop matches");
	m.parse("abcd != \"xyz\"");
	lemon.not_ok(m.matches(&tm), "abcd != xyz doesn't match");

	// testing regex matching
	m.parse("AAAA =~ \".\"");
	lemon.ok(m.matches(&tm), "AAAA =~ . matches");
	m.parse("AAAA =~ \"123\"");
	lemon.ok(m.matches(&tm), "AAAA =~ 123 matches");
	m.parse("AAAA =~ \"234\"");
	lemon.ok(m.matches(&tm), "AAAA =~ 234 matches");
	m.parse("AAAA =~ \"45\"");
	lemon.ok(m.matches(&tm), "AAAA =~ 45 matches");
	m.parse("AAAA =~ \"^12345$\"");
	lemon.ok(m.matches(&tm), "AAAA =~ ^12345$ matches");
	m.parse("AAAA =~ \"^123456$\"");
	lemon.not_ok(m.matches(&tm), "AAAA =~ ^123456$ doesn't match");

	m.parse("AAAA !~ \".\"");
	lemon.not_ok(m.matches(&tm), "AAAA !~ . doesn't match");
	m.parse("AAAA !~ \"123\"");
	lemon.is(m.matches(&tm), false, "AAAA !~ 123 doesn't match");
	m.parse("AAAA !~ \"234\"");
	lemon.is(m.matches(&tm), false, "AAAA !~ 234 doesn't match");
	m.parse("AAAA !~ \"45\"");
	lemon.is(m.matches(&tm), false, "AAAA !~ 45 doesn't match");
	m.parse("AAAA !~ \"^12345$\"");
	lemon.is(m.matches(&tm), false, "AAAA !~ ^12345$ doesn't match");

	// testing the "contains" operator
	m.parse("tags # \"foo\"");
	lemon.is(m.matches(&tm), true, "tags # foo matches");
	m.parse("tags # \"baz\"");
	lemon.is(m.matches(&tm), true, "tags # baz matches");
	m.parse("tags # \"quux\"");
	lemon.is(m.matches(&tm), true, "tags # quux matches");
	m.parse("tags # \"xyz\"");
	lemon.is(m.matches(&tm), false, "tags # xyz doesn't match");
	m.parse("tags # \"foo bar\"");
	lemon.is(m.matches(&tm), false, "tags # foo bar doesn't match");
	m.parse("tags # \"foo\" and tags # \"bar\"");
	lemon.is(m.matches(&tm), true, "tags # foo and tags # bar matches");
	m.parse("tags # \"foo\" and tags # \"xyz\"");
	lemon.is(m.matches(&tm), false, "tags # foo and tags # xyz doesn't match");
	m.parse("tags # \"foo\" or tags # \"xyz\"");
	lemon.is(m.matches(&tm), true, "tags # foo or tags # xyz matches");

	m.parse("tags !# \"nein\"");
	lemon.is(m.matches(&tm), true, "tags !# nein matches");
	m.parse("tags !# \"foo\"");
	lemon.is(m.matches(&tm), false, "tags !# foo doesn't match");

	m.parse("AAAA > 12344");
	lemon.is(m.matches(&tm), true, "AAAA > 12344 matches");
	m.parse("AAAA > 12345");
	lemon.is(m.matches(&tm), false, "AAAA > 12345 doesn't match");
	m.parse("AAAA >= 12345");
	lemon.is(m.matches(&tm), true, "AAAA >= 12345 matches");
	m.parse("AAAA < 12345");
	lemon.is(m.matches(&tm), false, "AAAA < 12345 doesn't match");
	m.parse("AAAA <= 12345");
	lemon.is(m.matches(&tm), true, "AAAA <= 12345 matches");

	m.parse("AAAA between 0:12345");
	lemon.is(m.matches(&tm), true, "AAAA between 0:12345 matches");
	m.parse("AAAA between 12345:12345");
	lemon.is(m.matches(&tm), true, "AAAA between 12345:12345 matches");
	m.parse("AAAA between 23:12344");
	lemon.is(m.matches(&tm), false, "AAAA between 23:12344 doesn't match");

	m.parse("AAAA between 0");
	lemon.is(m.matches(&tm), false, "invalid between expression (1)");

	lemon.is(m.parse("AAAA between 0:15:30"), false, "invalid between expression won't be parsed");
	lemon.ok(m.get_parse_error() != "", "invalid between expression leads to parse error");

	m.parse("AAAA between 1:23");
	lemon.ok(m.get_parse_error() == "", "valid between expression returns no parse error");

	m.parse("AAAA between 12346:12344");
	lemon.is(m.matches(&tm), true, "reverse ranges will match, too");

	try {
		m.parse("BBBB < 0");
		m.matches(&tm);
		lemon.fail("attribute BBBB shouldn't have been found");
	} catch (const matcherexception& e) {
		lemon.ok(true, "attribute BBBB was detected as non-existent");
	}

	try {
		m.parse("BBBB > 0");
		m.matches(&tm);
		lemon.fail("attribute BBBB shouldn't have been found");
	} catch (const matcherexception& e) {
		lemon.ok(true, "attribute BBBB was detected as non-existent");
	}

	try {
		m.parse("BBBB =~ \"foo\"");
		m.matches(&tm);
		lemon.fail("attribute BBBB shouldn't have been found");
	} catch (const matcherexception& e) {
		lemon.ok(true, "attribute BBBB was detected as non-existent");
	}

	try {
		m.parse("BBBB between 1:23");
		m.matches(&tm);
		lemon.fail("attribute BBBB shouldn't have been found");
	} catch (const matcherexception& e) {
		lemon.ok(true, "attribute BBBB was detected as non-existent");
	}

	try {
		m.parse("AAAA =~ \"[[\"");
		m.matches(&tm);
		lemon.fail("invalid regex should have been found");
	} catch (const matcherexception& e) {
		lemon.ok(true, "invalid regex was found");
	}

	try {
		m.parse("BBBB # \"foo\"");
		m.matches(&tm);
		lemon.fail("attribute BBBB shouldn't have been found");
	} catch (const matcherexception& e) {
		lemon.ok(true, "attribute BBBB was detected as non-existent");
	}

	matcher m2("AAAA between 1:30000");
	lemon.is(m2.get_expression(), "AAAA between 1:30000", "get_expression returns previously parsed expression");
}


void TestHistory() {
	history h;

	lemon.is(h.prev(), "", "previous is empty");
	lemon.is(h.prev(), "", "previous is empty");
	lemon.is(h.next(), "", "previous is empty");
	lemon.is(h.next(), "", "previous is empty");

	h.add_line("testline");
	lemon.is(h.prev(), "testline", "previous is testline");
	lemon.is(h.prev(), "testline", "previous is testline");
	lemon.is(h.next(), "testline", "previous is testline");
	lemon.is(h.next(), "", "next is empty");

	h.add_line("foobar");
	lemon.is(h.prev(), "foobar","previous is foobar");
	lemon.is(h.prev(), "testline", "previous is testline");
	lemon.is(h.next(), "testline", "previous is testline");
	lemon.is(h.prev(), "testline", "previous is testline");
	lemon.is(h.next(), "testline", "previous is testline");
	lemon.is(h.next(), "foobar", "next is foobar");
	lemon.is(h.next(), "", "next is empty");
	lemon.is(h.next(), "", "next is empty");
}

void TestStringConversion() {
	std::string s1 = utils::wstr2str(L"This is a simple string. Let's have a look at the outcome...");
	lemon.is(s1, "This is a simple string. Let's have a look at the outcome...", "conversion from wstring to string");

	std::wstring w1 = utils::str2wstr("And that's another simple string.");
	lemon.is(w1 == L"And that's another simple string.", true, "conversion from string to wstring");

	std::wstring w2 = utils::str2wstr("");
	lemon.is(w2 == L"", true, "conversion from empty string to wstring");

	std::string s2 = utils::wstr2str(L"");
	lemon.is(s2, "", "conversion from empty wstring to string");
	
}

void TestFmtStrFormatter() {
	fmtstr_formatter fmt;
	fmt.register_fmt('a', "AAA");
	fmt.register_fmt('b', "BBB");
	fmt.register_fmt('c', "CCC");
	lemon.is(fmt.do_format(""), "", "empty format string");
	lemon.is(fmt.do_format("%"), "", "format string with illegal single %");
	lemon.is(fmt.do_format("%%"), "%", "format string with %%");
	lemon.is(fmt.do_format("%a%b%c"), "AAABBBCCC", "format string substitution of %a%b%c");
	lemon.is(fmt.do_format("%%%a%%%b%%%c%%"), "%AAA%BBB%CCC%", "format string substitution of %a, %b, %c intermixed with %%");

	lemon.is(fmt.do_format("%4a")," AAA", "format string align right");
	lemon.is(fmt.do_format("%-4a"), "AAA ", "format string align left");

	lemon.is(fmt.do_format("%2a"), "AA", "format string limit size (align right)");
	lemon.is(fmt.do_format("%-2a"), "AA", "format string limit size (align left");

	lemon.is(fmt.do_format("<%a> <%5b> | %-5c%%"), "<AAA> <  BBB> | CCC  %", "complex format string");

	fmtstr_formatter fmt2;
	fmt2.register_fmt('a',"AAA");
	lemon.is(fmt2.do_format("%?a?%a&no?"), "AAA", "conditional format string (1)");
	lemon.is(fmt2.do_format("%?b?%b&no?"), "no", "conditional format string (2)");
	lemon.is(fmt2.do_format("%?a?[%-4a]&no?"), "[AAA ]", "complex format string with conditional");

	fmt2.register_fmt('b',"BBB");
	lemon.is(fmt2.do_format("asdf | %a | %?c?%a%b&%b%a? | qwert"), "asdf | AAA | BBBAAA | qwert", "complex format string (2)");
	lemon.is(fmt2.do_format("%?c?asdf?"), "", "conditional format string (3)");
	fmt2.register_fmt('c',"CCC");
	lemon.is(fmt2.do_format("asdf | %a | %?c?%a%b&%b%a? | qwert"), "asdf | AAA | AAABBB | qwert", "complex format string (3)");
	lemon.is(fmt2.do_format("%?c?asdf?"), "asdf", "conditional format string (4)");

	lemon.is(fmt.do_format("%>X", 3), "XXX", "format string filler");
	lemon.is(fmt.do_format("%a%> %b", 10), "AAA    BBB", "format string filler (2)");
	lemon.is(fmt.do_format("%a%> %b", 0), "AAA BBB", "format string filler (3) with with 0");
}

void TestMiscUtilsFunctions() {
	/* this test assumes some command line utilities to be installed */

	lemon.is(utils::get_command_output("ls /dev/null"), "/dev/null\n", "ls /dev/null");

	char * argv[4];
	argv[0] = "cat";
	argv[1] = NULL;
	lemon.is(utils::run_program(argv, "this is a multine-line\ntest string"), "this is a multine-line\ntest string", "run_program(cat)");
	argv[0] = "echo";
	argv[1] = "-n";
	argv[2] = "hello world";
	argv[3] = NULL;
	lemon.is(utils::run_program(argv, ""), "hello world", "echo -n 'hello world'");

	lemon.is(utils::replace_all("aaa", "a", "b"), "bbb", "replace all a by b");
	lemon.is(utils::replace_all("aaa", "aa", "ba"), "baa", "replace all aa by ba");
	lemon.is(utils::replace_all("aaaaaa", "aa", "ba"), "bababa", "replace all aa by ba (2)");
	lemon.is(utils::replace_all("", "a", "b"), "", "replace a by b on empty string");
	lemon.is(utils::replace_all("aaaa", "b", "c"), "aaaa", "replace b by c in string only consisting of a");
	lemon.is(utils::replace_all("this is a normal test text", " t", " T"), "this is a normal Test Text", "replace t by T");
	lemon.is(utils::replace_all("o o o", "o", "<o>"), "<o> <o> <o>", "replace o by <o>");

	lemon.is(utils::to_string<int>(0), "0", "convert 0 to string");
	lemon.is(utils::to_string<int>(100), "100", "convert 100 to string");
	lemon.is(utils::to_string<unsigned int>(65536), "65536", "convert 65536 to string");
	lemon.is(utils::to_string<unsigned int>(65537), "65537", "convert 65537 to string");
}

void TestUtilsStrPrintf() {
	lemon.is(utils::strprintf(NULL), "", "strprintf with NULL format string");
	lemon.is(utils::strprintf("%s", ""), "", "strprintf(%s) with empty string");
	lemon.is(utils::strprintf("%u", 0), "0", "strprintf(%u) with 0");
	lemon.is(utils::strprintf("%s", NULL), "(null)", "strprintf(%s) with NULL");
	lemon.is(utils::strprintf("%u-%s-%c", 23, "hello world", 'X'), "23-hello world-X", "strprintf(%u-%s-%c)");
}

void TestRegexManager() {

	regexmanager rxman;

	std::vector<std::string> params;
	try {
		rxman.handle_action("highlight", params);
		lemon.fail("highlight with no parameters threw no exception");
	} catch (const confighandlerexception& e) {
		lemon.pass("highlight with no parameters threw an exception");
	}
	params.push_back("articlelist");
	params.push_back("foo");
	params.push_back("blue");
	params.push_back("red");

	try {
		rxman.handle_action("highlight", params);
		lemon.pass("highlight with correct parameters threw no exception");
	} catch (const confighandlerexception& e) {
		lemon.fail("highlight with correct parameters threw an exception");
	}

	std::string str = "xfoox";
	rxman.quote_and_highlight(str, "articlelist");
	lemon.is(str, "x<0>foo</>x", "highlight of foo in articlelist");

	str = "xfoox";
	rxman.quote_and_highlight(str, "feedlist");
	lemon.is(str, "xfoox", "highlight of foo in feedlist");

	params[0] = "feedlist";
	try {
		rxman.handle_action("highlight", params);
		lemon.pass("highlight with correct parameters threw no exception");
	} catch (const confighandlerexception& e) {
		lemon.fail("highlight with correct parameters threw an exception");
	}

	str = "yfooy";
	rxman.quote_and_highlight(str, "feedlist");
	lemon.is(str, "y<0>foo</>y", "highlight of foo in feedlist");

	params[0] = "invalidloc";
	try {
		rxman.handle_action("highlight", params);
		lemon.fail("highlight with invalid location threw no exception");
	} catch (const confighandlerexception& e) {
		lemon.pass("highlight with invalid location threw an exception");
	}

	params[0] = "feedlist";
	params[1] = "*";
	try {
		rxman.handle_action("highlight", params);
		lemon.fail("highlight with invalid expression threw no exception");
	} catch (const confighandlerexception& e) {
		lemon.pass("highlight with invalid expression threw an exception");
	}

	params[1] = "foo";
	params.push_back("bold");
	params.push_back("underline");
	try {
		rxman.handle_action("highlight", params);
		lemon.pass("highlight with correct parameters threw no exception");
	} catch (const confighandlerexception& e) {
		lemon.fail("highlight with correct parameters threw an exception");
	}

	params[0] = "all";
	try {
		rxman.handle_action("highlight", params);
		lemon.pass("highlight with correct parameters threw no exception");
	} catch (const confighandlerexception& e) {
		lemon.fail("highlight with correct parameters threw an exception");
	}

	try {
		rxman.handle_action("an-invalid-command", params);
		lemon.fail("an-invalid-command threw no exception");
	} catch (const confighandlerexception& e) {
		lemon.pass("an-invalid-command threw an exception");
	}

	str = "<";
	rxman.quote_and_highlight(str, "feedlist");
	lemon.is(str, "<", "quote_and_highlight <");

	str = "a<b>";
	rxman.quote_and_highlight(str, "feedlist");
	lemon.is(str, "a<b>", "quote_and_highlight a<b>");
}
void TestHtmlRenderer() {
	htmlrenderer rnd(100);

	std::vector<std::string> lines;
	std::vector<linkpair> links;

	rnd.render("<a href=\"http://slashdot.org/\">slashdot</a>", lines, links, "");
	lemon.ok(lines.size() >= 1u, "rendering produced one line");
	lemon.is(lines[0], "<u>slashdot</>[1]", "first line contains underlined link");
	lemon.is(links[0].first, "http://slashdot.org/", "first link");
	lemon.is(links[0].second, LINK_HREF, "first link type");

	lines.erase(lines.begin(), lines.end());
	links.erase(links.begin(), links.end());

	rnd.render("hello<br />world!", lines, links, "");
	lemon.is(lines.size(), 2u, "text with <br> produces two lines");
	lemon.is(lines[0], "hello", "first line hello");
	lemon.is(lines[1], "world!", "second line is world!");

	lines.erase(lines.begin(), lines.end());
	links.erase(links.begin(), links.end());

	rnd.render("3<sup>10</sup>", lines, links, "");
	lemon.is(lines.size(), 1u, "superscription produced 1 line");
	lemon.is(lines[0], "3^10", "superscription of 3^10");

	lines.erase(lines.begin(), lines.end());
	links.erase(links.begin(), links.end());

	rnd.render("A<sub>i</sub>", lines, links, "");
	lemon.is(lines.size(), 1u, "subscription produced 1 line");
	lemon.is(lines[0], "A[i]", "subscription of A[i]");

	lines.erase(lines.begin(), lines.end());
	links.erase(links.begin(), links.end());

	rnd.render("abc<script></script>", lines, links, "");
	lemon.is(lines.size(), 1u, "rendering produced one line");
	lemon.is(lines.at(0),"abc", "rendering of abc<script></script> produced one empty line");
}

void TestIndexPartitioning() {
	std::vector<std::pair<unsigned int, unsigned int> > partitions = utils::partition_indexes(0, 9, 2);
	lemon.is(partitions.size(), 2u, "partitioning of [0,9] in 2 parts produced 2 parts");
	lemon.is(partitions[0].first, 0u, "first partition start is 0");
	lemon.is(partitions[0].second, 4u, "first partition end is 4");
	lemon.is(partitions[1].first, 5u, "second partition start is 5");
	lemon.is(partitions[1].second, 9u, "second partition end is 9");

	partitions = utils::partition_indexes(0, 10, 3);
	lemon.is(partitions.size(), 3u, "partitioning of [0,10] in 3 parts produced 3 parts");
	lemon.is(partitions[0].first, 0u, "first partition start is 0");
	lemon.is(partitions[0].second, 2u, "first partition end is 2");
	lemon.is(partitions[1].first, 3u, "second partition start is 3");
	lemon.is(partitions[1].second, 5u, "second partition end is 5");
	lemon.is(partitions[2].first, 6u, "third partition start is 6");
	lemon.is(partitions[2].second, 10u, "third partition end is 10");

	partitions = utils::partition_indexes(0, 11, 3);
	lemon.is(partitions.size(), 3u, "partitioning of [0,11] in 3 parts produced 3 parts");
	lemon.is(partitions[0].first, 0u, "first partition start is 0");
	lemon.is(partitions[0].second, 3u, "first partition end is 3");
	lemon.is(partitions[1].first, 4u, "second partition start is 4");
	lemon.is(partitions[1].second, 7u, "second partition end is 7");
	lemon.is(partitions[2].first, 8u, "third partition start is 8");
	lemon.is(partitions[2].second, 11u, "third partition end is 11");

	partitions = utils::partition_indexes(0, 199, 200);
	lemon.is(partitions.size(), 200u, "partitioning of [0,199] in 200 parts produced 200 parts");

	partitions = utils::partition_indexes(0, 103, 1);
	lemon.is(partitions.size(), 1u, "partitioning of [0,103] in 1 partition produced 1 partition");
	lemon.is(partitions[0].first, 0u, "first partition start is 0");
	lemon.is(partitions[0].second, 103u, "first partition end is 103");
}

void TestCensorUrl() {
	lemon.is(utils::censor_url(""), "", "censor empty string");
	lemon.is(utils::censor_url("foobar"), "foobar", "censor foobar");
	lemon.is(utils::censor_url("foobar://xyz/"), "foobar://xyz/", "censor foobar: url with no authinfo");

	lemon.is(utils::censor_url("http://newsbeuter.org/"), "http://newsbeuter.org/", "censor http url with no authinfo");
	lemon.is(utils::censor_url("https://newsbeuter.org/"), "https://newsbeuter.org/", "censor https url with no authinfo");

	lemon.is(utils::censor_url("http://@newsbeuter.org/"), "http://*:*@newsbeuter.org/", "censor http url with empty authinfo");
	lemon.is(utils::censor_url("https://@newsbeuter.org/"), "https://*:*@newsbeuter.org/", "censor https url with empty authinfo");

	lemon.is(utils::censor_url("http://foo:bar@newsbeuter.org/"), "http://*:*@newsbeuter.org/", "censor http url with authinfo");
	lemon.is(utils::censor_url("https://foo:bar@newsbeuter.org/"), "https://*:*@newsbeuter.org/", "censor https url with authinfo");

	lemon.is(utils::censor_url("http://aschas@newsbeuter.org/"), "http://*:*@newsbeuter.org/", "censor http url with username-only authinfo");
	lemon.is(utils::censor_url("https://aschas@newsbeuter.org/"), "https://*:*@newsbeuter.org/", "censor https url with username-only authinfo");

	lemon.is(utils::censor_url("xxx://aschas@newsbeuter.org/"), "xxx://*:*@newsbeuter.org/", "censor xxx url with username-only authinfo");

	lemon.is(utils::censor_url("http://foobar"), "http://foobar", "censor http url with no authinfo and no trailing slash");
	lemon.is(utils::censor_url("https://foobar"), "https://foobar", "censor https url with no authinfo and no trailing slash");

	lemon.is(utils::censor_url("http://aschas@host"), "http://*:*@host", "censor http url with username-only authinfo and no trailing slash");
	lemon.is(utils::censor_url("https://aschas@host"), "https://*:*@host", "censor http url with username-only authinfo and no trailing slash");

	lemon.is(utils::censor_url("query:name:age between 1:10"), "query:name:age between 1:10", "censor query feed");
}

void TestMakeAbsoluteUrl() {
	lemon.is(utils::absolute_url("http://foobar/hello/crook/", "bar.html"), "http://foobar/hello/crook/bar.html", "relative url");
	lemon.is(utils::absolute_url("https://foobar/foo/", "/bar.html"), "https://foobar/bar.html", "path-absolute url");
	lemon.is(utils::absolute_url("https://foobar/foo/", "http://quux/bar.html"), "http://quux/bar.html", "absolute url");
	lemon.is(utils::absolute_url("http://foobar", "bla.html"), "http://foobar/bla.html", "relative url with absolute url with no trailing slash");
	lemon.is(utils::absolute_url("http://test:test@foobar:33", "bla2.html"), "http://test:test@foobar:33/bla2.html", "relative url with absolute url with authinfo and port");
}

void TestBacktickEvaluation() {
	lemon.is(configparser::evaluate_backticks(""), "", "backtick evaluation of empty string");
	lemon.is(configparser::evaluate_backticks("hello world"), "hello world", "backtick evaluation of string with no backticks");
	lemon.is(configparser::evaluate_backticks("foo`true`baz"), "foobaz", "backtick evaluation with true (empty string)");
	lemon.is(configparser::evaluate_backticks("foo`barbaz"), "foo`barbaz", "backtick evaluation with missing second backtick");
	lemon.is(configparser::evaluate_backticks("foo `true` baz"), "foo  baz", "backtick evaluation with true (2)");
	lemon.is(configparser::evaluate_backticks("foo `true` baz `xxx"), "foo  baz `xxx", "backtick evaluation with missing third backtick");
	lemon.is(configparser::evaluate_backticks("`echo hello world`"), "hello world", "backtick evaluation of an echo command");
	lemon.is(configparser::evaluate_backticks("xxx`echo yyy`zzz"), "xxxyyyzzz", "backtick evaluation of an echo command embedded in regular text");
	lemon.is(configparser::evaluate_backticks("`echo 3 \\* 4 | bc`"), "12", "backtick evaluation of an expression piped into bc");
}

void TestUtilsQuote() {
	lemon.is(utils::quote(""), "\"\"", "quote empty string");
	lemon.is(utils::quote("hello world"), "\"hello world\"", "quote regular string");
	lemon.is(utils::quote("\"hello world\""), "\"\\\"hello world\\\"\"", "quote string that contains quotes");
}

void TestUtilsFunctions_to_u() {
	lemon.is(utils::to_u("0"), 0u, "conversion to unsigned 0 -> 0");
	lemon.is(utils::to_u("23"), 23u, "convertion to unsigned 23 -> 23");
	lemon.is(utils::to_u(""), 0u, "conversion to unsigned empty string -> 0");
}

void TestUtilsFunctions_strwidth() {
	lemon.is(utils::strwidth(""), 0u, "empty string is 0 colums wide");
	lemon.is(utils::strwidth("xx"), 2u, "xx is 2 columns wide");
	lemon.is(utils::strwidth(utils::wstr2str(L"\uF91F")), 2u, "character U+F91F is 2 columns wide");
}

void TestUtilsFunction_join() {
	std::vector<std::string> str;
	lemon.is(utils::join(str, ""), "", "join of no elements with empty string");
	lemon.is(utils::join(str, "-"), "", "join of no elements with -");
	str.push_back("foobar");
	lemon.is(utils::join(str, ""), "foobar", "join of single element with empty string");
	lemon.is(utils::join(str, "-"), "foobar", "join of single element with -");
	str.push_back("quux");
	lemon.is(utils::join(str, ""), "foobarquux", "join of two elements two empty string");
	lemon.is(utils::join(str, "-"), "foobar-quux", "join of two elements with -");
}

void TestUtilsFunction_trim() {
	std::string str = "  xxx\r\n";
	utils::trim(str);
	lemon.is(str, "xxx", "trim string starting with spaces and ending with CRLF");
	str = "\n\n abc  foobar\n";
	utils::trim(str);
	lemon.is(str, "abc  foobar", "trim string starting with newlines and space and ending with newline");
	str = "";
	utils::trim(str);
	lemon.is(str, "", "trim empty string");
	str = "     \n";
	utils::trim(str);
	lemon.is(str, "", "trim string only consisting of spaces and newline");
	str = "quux\n";
	utils::trim_end(str);
	lemon.is(str, "quux", "trim end of string that ends with newline");
}

void TestOlFormatting() {
	htmlrenderer r;
	lemon.is(r.format_ol_count(1, '1'), " 1", "1 in digit formats to \"1\"");
	lemon.is(r.format_ol_count(3, '1'), " 3", "3 in digit formats to \"3\"");
	lemon.is(r.format_ol_count(3, 'a'), "c", "3 in alphabetic formats to \"c\"");
	lemon.is(r.format_ol_count(26 + 3, 'a'), "ac", "26+3 in alphabetic formats to \"ac\"");
	lemon.is(r.format_ol_count(3*26*26 + 5*26 + 2, 'a'), "ceb", "3*26*26 + 5*26 + 2 in alphabetic formats to \"ceb\"");

	lemon.is(r.format_ol_count(3, 'A'), "C", "3 in alphabetic uppercase formats to \"C\"");
	lemon.is(r.format_ol_count(26 + 5, 'A'), "AE", "26+5 in alphabetic uppercase formats to \"AE\"");
	lemon.is(r.format_ol_count(27, 'A'), "AA", "27 in alphabetic uppercase formats to \"AA\"");
	lemon.is(r.format_ol_count(26, 'A'), "Z", "26 in alphabetic uppercase formats to \"Z\"");
	lemon.is(r.format_ol_count(26*26+26, 'A'), "ZZ", "26*26+26 in alphabetic uppercase formats to \"ZZ\"");
	lemon.is(r.format_ol_count(25*26*26 + 26*26+26, 'A'), "YZZ", "26*26*26+26*26+26 in alphabetic uppercase formats to \"YZZ\"");

	lemon.is(r.format_ol_count(1, 'i'), "i", "1 in roman numerals is i");
	lemon.is(r.format_ol_count(2, 'i'), "ii", "2 in roman numerals is ii");
	lemon.is(r.format_ol_count(5, 'i'), "v", "5 in roman numerals is v");
	lemon.is(r.format_ol_count(4, 'i'), "iv", "4 in roman numerals is iv");
	lemon.is(r.format_ol_count(6, 'i'), "vi", "6 in roman numerals is vi");
	lemon.is(r.format_ol_count(7, 'i'), "vii", "6 in roman numerals is vii");
	lemon.is(r.format_ol_count(10, 'i'), "x", "10 in roman numerals is x");

	lemon.is(r.format_ol_count(32, 'i'), "xxxii", "32 in roman numerals is xxxii");
	lemon.is(r.format_ol_count(1972, 'i'), "mcmlxxii", "1972 in roman numerals is mcmlxxii");

	lemon.is(r.format_ol_count(2011, 'I'), "MMXI", "201 in roman numerals uppercase is MMXI");
}

} // namespace test

int main(void) {
	setlocale(LC_CTYPE, "");
	logger::getInstance().set_logfile("testlog.txt");
	logger::getInstance().set_loglevel(LOG_DEBUG);

	test::TestNewsbeuterReload();
	test::TestConfigParserContainerAndKeymap();
	test::TestTagSoupPullParser();
	test::TestUrlReader();
	test::TestTokenizers();
	test::TestFilterLanguage();
	test::TestIssue94Crash();
	test::TestHistory();
	test::TestStringConversion();
	test::TestFmtStrFormatter();
	test::TestMiscUtilsFunctions();
	test::TestUtilsStrPrintf();
	test::TestRegexManager();
	test::TestHtmlRenderer();
	test::TestIndexPartitioning();
	test::TestCensorUrl();
	test::TestMakeAbsoluteUrl();
	test::TestBacktickEvaluation();
	test::TestUtilsQuote();
	test::TestUtilsFunctions_to_u();
	test::TestUtilsFunctions_strwidth();
	test::TestUtilsFunction_join();
	test::TestUtilsFunction_trim();
	test::TestOlFormatting();

	return test::lemon.done() ? 0 : 1;
}

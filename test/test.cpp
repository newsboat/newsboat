/* test driver for newsbeuter */

#define BOOST_AUTO_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#include <climits>
#include <vector>
#include <string>
#include <boost/test/auto_unit_test.hpp>

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

BOOST_AUTO_TEST_CASE(InitTests) {
	setlocale(LC_CTYPE, "");
	setlocale(LC_MESSAGES, "");
	GetLogger().set_logfile("testlog.txt");
	GetLogger().set_loglevel(LOG_DEBUG);
}

BOOST_AUTO_TEST_CASE(TestNewsbeuterReload) {
	configcontainer * cfg = new configcontainer();
	cache * rsscache = new cache("test-cache.db", cfg);

	rss_parser parser("http://bereshit.synflood.at/~ak/rss.xml", rsscache, cfg, NULL);
	std::tr1::shared_ptr<rss_feed> feed = parser.parse();
	BOOST_CHECK_EQUAL(feed->items().size(), 8u);

	rsscache->externalize_rssfeed(feed, false);
	rsscache->internalize_rssfeed(feed);
	BOOST_CHECK_EQUAL(feed->items().size(), 8u);

	BOOST_CHECK_EQUAL(feed->items()[0]->title(), "Teh Saxxi");
	BOOST_CHECK_EQUAL(feed->items()[7]->title(), "Handy als IR-Detektor");

	feed->items()[0]->set_title("Another Title");
	feed->items()[0]->set_pubDate(time(NULL));
	BOOST_CHECK_EQUAL(feed->items()[0]->title(), "Another Title");

	rsscache->externalize_rssfeed(feed, false);

	std::tr1::shared_ptr<rss_feed> feed2(new rss_feed(rsscache));
	feed2->set_rssurl("http://bereshit.synflood.at/~ak/rss.xml");
	rsscache->internalize_rssfeed(feed2);

	BOOST_CHECK_EQUAL(feed2->items().size(), 8u);
	BOOST_CHECK_EQUAL(feed2->items()[0]->title(), "Another Title");
	BOOST_CHECK_EQUAL(feed2->items()[7]->title(), "Handy als IR-Detektor");

	rsscache->set_lastmodified("http://bereshit.synflood.at/~ak/rss.xml", 1000);
	BOOST_CHECK_EQUAL(rsscache->get_lastmodified("http://bereshit.synflood.at/~ak/rss.xml"), 1000);
	rsscache->set_lastmodified("http://bereshit.synflood.at/~ak/rss.xml", 0);
	BOOST_CHECK_EQUAL(rsscache->get_lastmodified("http://bereshit.synflood.at/~ak/rss.xml"), 1000);

	std::vector<std::string> feedurls = rsscache->get_feed_urls();
	BOOST_CHECK_EQUAL(feedurls.size(), 1u);
	BOOST_CHECK_EQUAL(feedurls[0], "http://bereshit.synflood.at/~ak/rss.xml");

	std::vector<std::tr1::shared_ptr<rss_feed> > feedv;
	feedv.push_back(feed);

	cfg->set_configvalue("cleanup-on-quit", "true");
	rsscache->cleanup_cache(feedv);

	delete rsscache;
	delete cfg;

	::unlink("test-cache.db");
}

BOOST_AUTO_TEST_CASE(TestConfigParserContainerAndKeymap) {
	configcontainer * cfg = new configcontainer();
	configparser cfgparser;
	cfg->register_commands(cfgparser);
	keymap k(KM_NEWSBEUTER);
	cfgparser.register_handler("macro", &k);

	try {
		cfgparser.parse("test-config.txt");
	}  catch (const configexception& ex) {
		LOG(LOG_ERROR,"an exception occured while parsing the configuration file: %s", ex.what());
		BOOST_CHECK(false);
	} catch (...) {
		BOOST_CHECK(false);
	}

	// test boolean config values
	BOOST_CHECK_EQUAL(cfg->get_configvalue("show-read-feeds"), "no");
	BOOST_CHECK_EQUAL(cfg->get_configvalue_as_bool("show-read-feeds"), false);

	// test string config values
	BOOST_CHECK_EQUAL(cfg->get_configvalue("browser"), "firefox");

	// test integer config values
	BOOST_CHECK_EQUAL(cfg->get_configvalue("max-items"), "100");
	BOOST_CHECK_EQUAL(cfg->get_configvalue_as_int("max-items"), 100);

	// test ~/ expansion for path config values
	std::string cachefilecomp = ::getenv("HOME");
	cachefilecomp.append("/foo");
	BOOST_CHECK(cfg->get_configvalue("cache-file") == cachefilecomp);

	delete cfg;

	BOOST_CHECK_EQUAL(k.get_operation("ENTER", "feedlist"), OP_OPEN);
	BOOST_CHECK_EQUAL(k.get_operation("u", "article"), OP_SHOWURLS);
	BOOST_CHECK_EQUAL(k.get_operation("X", "feedlist"), OP_NIL);
	BOOST_CHECK_EQUAL(k.get_operation("", "feedlist"), OP_NIL);

	k.unset_key("ENTER", "all");
	BOOST_CHECK_EQUAL(k.get_operation("ENTER", "all"), OP_NIL);
	k.set_key(OP_OPEN, "ENTER", "all");
	BOOST_CHECK_EQUAL(k.get_operation("ENTER", "all"), OP_OPEN);

	BOOST_CHECK_EQUAL(k.get_opcode("open"), OP_OPEN);
	BOOST_CHECK_EQUAL(k.get_opcode("some-noexistent-operation"), OP_NIL);

	BOOST_CHECK_EQUAL(k.getkey(OP_OPEN, "all"), "ENTER");
	BOOST_CHECK_EQUAL(k.getkey(OP_TOGGLEITEMREAD, "all"), "N");
	BOOST_CHECK_EQUAL(k.getkey(static_cast<operation>(30000), "all"), "<none>");

	BOOST_CHECK_EQUAL(k.get_key(" "), ' ');
	BOOST_CHECK_EQUAL(k.get_key("U"), 'U');
	BOOST_CHECK_EQUAL(k.get_key("~"), '~');
	BOOST_CHECK_EQUAL(k.get_key("INVALID"), 0);
	BOOST_CHECK_EQUAL(k.get_key("ENTER"), '\n');
	BOOST_CHECK_EQUAL(k.get_key("ESC"), '\033');
	BOOST_CHECK_EQUAL(k.get_key("^A"), '\001');

	std::vector<std::string> params;
	try {
		k.handle_action("bind-key", params);
	} catch (const confighandlerexception& e) {
		BOOST_CHECK(std::string(e.what()).length() > 0);
	}
	try {
		k.handle_action("unbind-key", params);
	} catch (const confighandlerexception& e) {
		BOOST_CHECK(std::string(e.what()).length() > 0);
	}
	try {
		k.handle_action("macro", params);
	} catch (const confighandlerexception& e) {
		BOOST_CHECK(std::string(e.what()).length() > 0);
	}
	params.push_back("r");
	try {
		k.handle_action("bind-key", params);
	} catch (const confighandlerexception& e) {
		BOOST_CHECK(std::string(e.what()).length() > 0);
	}
	try {
		k.handle_action("unbind-key", params);
	} catch (const confighandlerexception& e) {
		BOOST_CHECK_EQUAL(std::string(e.what()).length(), 0u);
	}
	params.push_back("open");
	try {
		k.handle_action("bind-key", params);
	} catch (const confighandlerexception& e) {
		BOOST_CHECK_EQUAL(std::string(e.what()).length(), 0u);
	}
	try {
		k.handle_action("an-invalid-action", params);
	} catch (const confighandlerexception& e) {
		BOOST_CHECK(std::string(e.what()).length() > 0);
	}
}

BOOST_AUTO_TEST_CASE(TestTagSoupPullParser) {
	std::istringstream is("<test><foo quux='asdf' bar=\"qqq\">text</foo>more text<more>&quot;&#33;&#x40;</more></test>");
	tagsouppullparser xpp;
	tagsouppullparser::event e;
	xpp.setInput(is);

	e = xpp.getEventType();
	BOOST_CHECK_EQUAL(e, tagsouppullparser::START_DOCUMENT);
	e = xpp.next();
	BOOST_CHECK_EQUAL(e, tagsouppullparser::START_TAG);
	BOOST_CHECK_EQUAL(xpp.getText(), "test");
	e = xpp.next();
	BOOST_CHECK_EQUAL(e, tagsouppullparser::START_TAG);
	BOOST_CHECK_EQUAL(xpp.getText(), "foo");
	BOOST_CHECK_EQUAL(xpp.getAttributeValue("quux"), "asdf");
	BOOST_CHECK_EQUAL(xpp.getAttributeValue("bar"), "qqq");
	e = xpp.next();
	BOOST_CHECK_EQUAL(e, tagsouppullparser::TEXT);
	BOOST_CHECK_EQUAL(xpp.getText(), "text");
	e = xpp.next();
	BOOST_CHECK_EQUAL(e, tagsouppullparser::END_TAG);
	BOOST_CHECK_EQUAL(xpp.getText(), "foo");
	e = xpp.next();
	BOOST_CHECK_EQUAL(e, tagsouppullparser::TEXT);
	BOOST_CHECK_EQUAL(xpp.getText(), "more text");
	e = xpp.next();
	BOOST_CHECK_EQUAL(e, tagsouppullparser::START_TAG);
	BOOST_CHECK_EQUAL(xpp.getText(), "more");
	e = xpp.next();
	BOOST_CHECK_EQUAL(e, tagsouppullparser::TEXT);
	BOOST_CHECK_EQUAL(xpp.getText(), "\"!@");
	e = xpp.next();
	BOOST_CHECK_EQUAL(e, tagsouppullparser::END_TAG);
	BOOST_CHECK_EQUAL(xpp.getText(), "more");
	e = xpp.next();
	BOOST_CHECK_EQUAL(e, tagsouppullparser::END_TAG);
	BOOST_CHECK_EQUAL(xpp.getText(), "test");
	e = xpp.next();
	BOOST_CHECK_EQUAL(e, tagsouppullparser::END_DOCUMENT);
	e = xpp.next();
	BOOST_CHECK_EQUAL(e, tagsouppullparser::END_DOCUMENT);
}

BOOST_AUTO_TEST_CASE(TestUrlReader) {
	file_urlreader u;
	u.load_config("test-urls.txt");
	BOOST_CHECK_EQUAL(u.get_urls().size(), 3u);
	BOOST_CHECK_EQUAL(u.get_urls()[0], "http://test1.url.cc/feed.xml");
	BOOST_CHECK_EQUAL(u.get_urls()[1], "http://anotherfeed.com/");
	BOOST_CHECK_EQUAL(u.get_urls()[2], "http://onemorefeed.at/feed/");

	BOOST_CHECK_EQUAL(u.get_tags("http://test1.url.cc/feed.xml").size(), 2u);
	BOOST_CHECK_EQUAL(u.get_tags("http://test1.url.cc/feed.xml")[0], "tag1");
	BOOST_CHECK_EQUAL(u.get_tags("http://test1.url.cc/feed.xml")[1], "tag2");
	BOOST_CHECK_EQUAL(u.get_tags("http://anotherfeed.com/").size(), 0u);
	BOOST_CHECK_EQUAL(u.get_tags("http://onemorefeed.at/feed/").size(), 2u);

	BOOST_CHECK_EQUAL(u.get_alltags().size(), 3u);
}

BOOST_AUTO_TEST_CASE(TestTokenizers) {
	std::vector<std::string> tokens;

	tokens = utils::tokenize("as df qqq");
	BOOST_CHECK_EQUAL(tokens.size(), 3u);
	BOOST_CHECK(tokens[0] == "as" && tokens[1] == "df" && tokens[2] == "qqq");

	tokens = utils::tokenize(" aa ");
	BOOST_CHECK_EQUAL(tokens.size(), 1u);
	BOOST_CHECK_EQUAL(tokens[0], "aa");

	tokens = utils::tokenize("	");
	BOOST_CHECK_EQUAL(tokens.size(), 0u);
	
	tokens = utils::tokenize("");
	BOOST_CHECK_EQUAL(tokens.size(), 0u);

	tokens = utils::tokenize_spaced("a b");
	BOOST_CHECK_EQUAL(tokens.size(), 3u);
	BOOST_CHECK_EQUAL(tokens[1], " ");

	tokens = utils::tokenize_spaced(" a\t b ");
	BOOST_CHECK_EQUAL(tokens.size(), 5u);
	BOOST_CHECK_EQUAL(tokens[0], " ");
	BOOST_CHECK_EQUAL(tokens[1], "a");
	BOOST_CHECK_EQUAL(tokens[2], " ");

	tokens = utils::tokenize_quoted("asdf \"foobar bla\" \"foo\\r\\n\\tbar\"");
	BOOST_CHECK_EQUAL(tokens.size(), 3u);
	BOOST_CHECK_EQUAL(tokens[0], "asdf");
	BOOST_CHECK_EQUAL(tokens[1], "foobar bla");
	BOOST_CHECK_EQUAL(tokens[2], "foo\r\n\tbar");

	tokens = utils::tokenize_quoted("  \"foo \\\\xxx\"\t\r \" \"");
	BOOST_CHECK_EQUAL(tokens.size(), 2u);
	BOOST_CHECK_EQUAL(tokens[0], "foo \\xxx");
	BOOST_CHECK_EQUAL(tokens[1], " ");

	tokens = utils::tokenize_quoted("\"\\\\");
	BOOST_CHECK_EQUAL(tokens.size(), 1u);
	BOOST_CHECK_EQUAL(tokens[0], "\\");

	// the following test cases specifically demonstrate a problem of the tokenize_quoted with several \\ sequences directly appended
	tokens = utils::tokenize_quoted("\"\\\\\\\\");
	BOOST_CHECK_EQUAL(tokens.size(), 1u);
	BOOST_CHECK_EQUAL(tokens[0], "\\\\");

	tokens = utils::tokenize_quoted("\"\\\\\\\\\\\\");
	BOOST_CHECK_EQUAL(tokens.size(), 1u);
	BOOST_CHECK_EQUAL(tokens[0], "\\\\\\");

	tokens = utils::tokenize_quoted("\"\\\\\\\\\"");
	BOOST_CHECK_EQUAL(tokens.size(), 1u);
	BOOST_CHECK_EQUAL(tokens[0], "\\\\");

	tokens = utils::tokenize_quoted("\"\\\\\\\\\\\\\"");
	BOOST_CHECK_EQUAL(tokens.size(), 1u);
	BOOST_CHECK_EQUAL(tokens[0], "\\\\\\");
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

BOOST_AUTO_TEST_CASE(TestFilterLanguage) {

	FilterParser fp;

	// test parser
	BOOST_CHECK_MESSAGE(fp.parse_string("a = \"b\""), utils::wstr2str(fp.get_error()).c_str());
	BOOST_CHECK_MESSAGE(!fp.parse_string("a = \"b"), utils::wstr2str(fp.get_error()).c_str());
	BOOST_CHECK_MESSAGE(!fp.parse_string("a = b"), utils::wstr2str(fp.get_error()).c_str());
	BOOST_CHECK_MESSAGE(fp.parse_string("(a=\"b\")"), utils::wstr2str(fp.get_error()).c_str());
	BOOST_CHECK_MESSAGE(fp.parse_string("((a=\"b\"))"), utils::wstr2str(fp.get_error()).c_str());
	BOOST_CHECK_MESSAGE(!fp.parse_string("((a=\"b\")))"), utils::wstr2str(fp.get_error()).c_str());

	// test operators
	BOOST_CHECK_MESSAGE(fp.parse_string("a != \"b\""), utils::wstr2str(fp.get_error()).c_str());
	BOOST_CHECK_MESSAGE(fp.parse_string("a =~ \"b\""), utils::wstr2str(fp.get_error()).c_str());
	BOOST_CHECK_MESSAGE(fp.parse_string("a !~ \"b\""), utils::wstr2str(fp.get_error()).c_str());
	BOOST_CHECK_MESSAGE(!fp.parse_string("a !! \"b\""), utils::wstr2str(fp.get_error()).c_str());

	// complex query
	BOOST_CHECK_EQUAL(fp.parse_string("( a = \"b\") and ( b = \"c\" ) or ( ( c != \"d\" ) and ( c !~ \"asdf\" )) or c != \"xx\""), true);

	testmatchable tm;
	matcher m;

	m.parse("abcd = \"xyz\"");
	BOOST_CHECK_EQUAL(m.matches(&tm), true);
	m.parse("abcd = \"uiop\"");
	BOOST_CHECK_EQUAL(m.matches(&tm), false);
	m.parse("abcd != \"uiop\"");
	BOOST_CHECK_EQUAL(m.matches(&tm), true);
	m.parse("abcd != \"xyz\"");
	BOOST_CHECK_EQUAL(m.matches(&tm), false);

	// testing regex matching
	m.parse("AAAA =~ \".\"");
	BOOST_CHECK_EQUAL(m.matches(&tm), true);
	m.parse("AAAA =~ \"123\"");
	BOOST_CHECK_EQUAL(m.matches(&tm), true);
	m.parse("AAAA =~ \"234\"");
	BOOST_CHECK_EQUAL(m.matches(&tm), true);
	m.parse("AAAA =~ \"45\"");
	BOOST_CHECK_EQUAL(m.matches(&tm), true);
	m.parse("AAAA =~ \"^12345$\"");
	BOOST_CHECK_EQUAL(m.matches(&tm), true);
	m.parse("AAAA =~ \"^123456$\"");
	BOOST_CHECK_EQUAL(m.matches(&tm), false);

	m.parse("AAAA !~ \".\"");
	BOOST_CHECK_EQUAL(m.matches(&tm), false);
	m.parse("AAAA !~ \"123\"");
	BOOST_CHECK_EQUAL(m.matches(&tm), false);
	m.parse("AAAA !~ \"234\"");
	BOOST_CHECK_EQUAL(m.matches(&tm), false);
	m.parse("AAAA !~ \"45\"");
	BOOST_CHECK_EQUAL(m.matches(&tm), false);
	m.parse("AAAA !~ \"^12345$\"");
	BOOST_CHECK_EQUAL(m.matches(&tm), false);

	// testing the "contains" operator
	m.parse("tags # \"foo\"");
	BOOST_CHECK_EQUAL(m.matches(&tm), true);
	m.parse("tags # \"baz\"");
	BOOST_CHECK_EQUAL(m.matches(&tm), true);
	m.parse("tags # \"quux\"");
	BOOST_CHECK_EQUAL(m.matches(&tm), true);
	m.parse("tags # \"xyz\"");
	BOOST_CHECK_EQUAL(m.matches(&tm), false);
	m.parse("tags # \"foo bar\"");
	BOOST_CHECK_EQUAL(m.matches(&tm), false);
	m.parse("tags # \"foo\" and tags # \"bar\"");
	BOOST_CHECK_EQUAL(m.matches(&tm), true);
	m.parse("tags # \"foo\" and tags # \"xyz\"");
	BOOST_CHECK_EQUAL(m.matches(&tm), false);
	m.parse("tags # \"foo\" or tags # \"xyz\"");
	BOOST_CHECK_EQUAL(m.matches(&tm), true);

	m.parse("AAAA > 12344");
	BOOST_CHECK_EQUAL(m.matches(&tm), true);
	m.parse("AAAA > 12345");
	BOOST_CHECK_EQUAL(m.matches(&tm), false);
	m.parse("AAAA >= 12345");
	BOOST_CHECK_EQUAL(m.matches(&tm), true);
	m.parse("AAAA < 12345");
	BOOST_CHECK_EQUAL(m.matches(&tm), false);
	m.parse("AAAA <= 12345");
	BOOST_CHECK_EQUAL(m.matches(&tm), true);
}

BOOST_AUTO_TEST_CASE(TestFilterLanguageMemMgmt) {
	matcher m1, m2;
	m1 = m2;
	m2.parse("tags # \"foo\"");
	m1 = m2;
}

BOOST_AUTO_TEST_CASE(TestHistory) {
	history h;

	BOOST_CHECK_EQUAL(h.prev(), "");
	BOOST_CHECK_EQUAL(h.prev(), "");
	BOOST_CHECK_EQUAL(h.next(), "");
	BOOST_CHECK_EQUAL(h.next(), "");

	h.add_line("testline");
	BOOST_CHECK_EQUAL(h.prev(), "testline");
	BOOST_CHECK_EQUAL(h.prev(), "testline");
	BOOST_CHECK_EQUAL(h.next(), "testline");
	BOOST_CHECK_EQUAL(h.next(), "");

	h.add_line("foobar");
	BOOST_CHECK_EQUAL(h.prev(), "foobar");
	BOOST_CHECK_EQUAL(h.prev(), "testline");
	BOOST_CHECK_EQUAL(h.next(), "testline");
	BOOST_CHECK_EQUAL(h.prev(), "testline");
	BOOST_CHECK_EQUAL(h.prev(), "testline");
	BOOST_CHECK_EQUAL(h.prev(), "testline");
	BOOST_CHECK_EQUAL(h.next(), "testline");
	BOOST_CHECK_EQUAL(h.prev(), "testline");
	BOOST_CHECK_EQUAL(h.next(), "testline");
	BOOST_CHECK_EQUAL(h.next(), "foobar");
	BOOST_CHECK_EQUAL(h.next(), "");
	BOOST_CHECK_EQUAL(h.next(), "");
}

BOOST_AUTO_TEST_CASE(TestStringConversion) {
	std::string s1 = utils::wstr2str(L"This is a simple string. Let's have a look at the outcome...");
	BOOST_CHECK_EQUAL(s1, "This is a simple string. Let's have a look at the outcome...");

	std::wstring w1 = utils::str2wstr("And that's another simple string.");
	BOOST_CHECK_EQUAL(w1 == L"And that's another simple string.", true);

	std::wstring w2 = utils::str2wstr("");
	BOOST_CHECK_EQUAL(w2 == L"", true);

	std::string s2 = utils::wstr2str(L"");
	BOOST_CHECK_EQUAL(s2, "");
	
}

BOOST_AUTO_TEST_CASE(TestFmtStrFormatter) {
	fmtstr_formatter fmt;
	fmt.register_fmt('a', "AAA");
	fmt.register_fmt('b', "BBB");
	fmt.register_fmt('c', "CCC");
	BOOST_CHECK_EQUAL(fmt.do_format(""), "");
	BOOST_CHECK_EQUAL(fmt.do_format("%"), "");
	BOOST_CHECK_EQUAL(fmt.do_format("%%"), "%");
	BOOST_CHECK_EQUAL(fmt.do_format("%a%b%c"), "AAABBBCCC");
	BOOST_CHECK_EQUAL(fmt.do_format("%%%a%%%b%%%c%%"), "%AAA%BBB%CCC%");

	BOOST_CHECK_EQUAL(fmt.do_format("%4a")," AAA");
	BOOST_CHECK_EQUAL(fmt.do_format("%-4a"), "AAA ");

	BOOST_CHECK_EQUAL(fmt.do_format("%2a"), "AA");
	BOOST_CHECK_EQUAL(fmt.do_format("%-2a"), "AA");

	BOOST_CHECK_EQUAL(fmt.do_format("<%a> <%5b> | %-5c%%"), "<AAA> <  BBB> | CCC  %");

	fmtstr_formatter fmt2;
	fmt2.register_fmt('a',"AAA");
	BOOST_CHECK_EQUAL(fmt2.do_format("%?a?%a&no?"), "AAA");
	BOOST_CHECK_EQUAL(fmt2.do_format("%?b?%b&no?"), "no");
	BOOST_CHECK_EQUAL(fmt2.do_format("%?a?[%-4a]&no?"), "[AAA ]");

	fmt2.register_fmt('b',"BBB");
	BOOST_CHECK_EQUAL(fmt2.do_format("asdf | %a | %?c?%a%b&%b%a? | qwert"), "asdf | AAA | BBBAAA | qwert");
	BOOST_CHECK_EQUAL(fmt2.do_format("%?c?asdf?"), "");
	fmt2.register_fmt('c',"CCC");
	BOOST_CHECK_EQUAL(fmt2.do_format("asdf | %a | %?c?%a%b&%b%a? | qwert"), "asdf | AAA | AAABBB | qwert");
	BOOST_CHECK_EQUAL(fmt2.do_format("%?c?asdf?"), "asdf");

	BOOST_CHECK_EQUAL(fmt.do_format("%>X", 3), "XXX");
	BOOST_CHECK_EQUAL(fmt.do_format("%a%> %b", 10), "AAA    BBB");
	BOOST_CHECK_EQUAL(fmt.do_format("%a%> %b", 0), "AAA BBB");

}

BOOST_AUTO_TEST_CASE(TestMiscUtilsFunctions) {
	/* this test assumes some command line utilities to be installed */

	BOOST_CHECK_EQUAL(utils::get_command_output("ls /dev/null"), "/dev/null\n");

	char * argv[4];
	argv[0] = "cat";
	argv[1] = NULL;
	BOOST_CHECK_EQUAL(utils::run_program(argv, "this is a multine-line\ntest string"), "this is a multine-line\ntest string");
	argv[0] = "echo";
	argv[1] = "-n";
	argv[2] = "hello world";
	argv[3] = NULL;
	BOOST_CHECK_EQUAL(utils::run_program(argv, ""), "hello world");

	BOOST_CHECK_EQUAL(utils::replace_all("aaa", "a", "b"), "bbb");
	BOOST_CHECK_EQUAL(utils::replace_all("aaa", "aa", "ba"), "baa");
	BOOST_CHECK_EQUAL(utils::replace_all("aaaaaa", "aa", "ba"), "bababa");
	BOOST_CHECK_EQUAL(utils::replace_all("", "a", "b"), "");
	BOOST_CHECK_EQUAL(utils::replace_all("aaaa", "b", "c"), "aaaa");
	BOOST_CHECK_EQUAL(utils::replace_all("this is a normal test text", " t", " T"), "this is a normal Test Text");

	BOOST_CHECK_EQUAL(utils::to_s(0), "0");
	BOOST_CHECK_EQUAL(utils::to_s(100), "100");
	BOOST_CHECK_EQUAL(utils::to_s(65536), "65536");
	BOOST_CHECK_EQUAL(utils::to_s(65537), "65537");

	std::vector<std::string> foobar;
	foobar.erase(foobar.begin(), foobar.end());
}

BOOST_AUTO_TEST_CASE(TestUtilsStrPrintf) {
	BOOST_CHECK_EQUAL(utils::strprintf(NULL), "");
	BOOST_CHECK_EQUAL(utils::strprintf("%s", ""), "");
	BOOST_CHECK_EQUAL(utils::strprintf("%u", 0), "0");
	BOOST_CHECK_EQUAL(utils::strprintf("%s", NULL), "(null)");
	BOOST_CHECK_EQUAL(utils::strprintf("%u-%s-%c", 23, "hello world", 'X'), "23-hello world-X");
}

BOOST_AUTO_TEST_CASE(TestRegexManager) {

	regexmanager rxman;

	std::vector<std::string> params;
	try {
		rxman.handle_action("highlight", params);
	} catch (const confighandlerexception& e) {
		BOOST_CHECK(std::string(e.what()).length() > 0);
	}
	params.push_back("articlelist");
	params.push_back("foo");
	params.push_back("blue");
	params.push_back("red");

	try {
		rxman.handle_action("highlight", params);
	} catch (const confighandlerexception& e) {
		BOOST_CHECK_EQUAL(std::string(e.what()).length(), 0u);
	}

	std::string str = "xfoox";
	rxman.quote_and_highlight(str, "articlelist");
	BOOST_CHECK_EQUAL(str, "x<0>foo</>x");

	str = "xfoox";
	rxman.quote_and_highlight(str, "feedlist");
	BOOST_CHECK_EQUAL(str, "xfoox");

	params[0] = "feedlist";
	try {
		rxman.handle_action("highlight", params);
	} catch (const confighandlerexception& e) {
		BOOST_CHECK_EQUAL(std::string(e.what()).length(), 0u);
	}

	str = "yfooy";
	rxman.quote_and_highlight(str, "feedlist");
	BOOST_CHECK_EQUAL(str, "y<0>foo</>y");

	params[0] = "invalidloc";
	try {
		rxman.handle_action("highlight", params);
	} catch (const confighandlerexception& e) {
		BOOST_CHECK(std::string(e.what()).length() > 0);
	}

	params[0] = "feedlist";
	params[1] = "*";
	try {
		rxman.handle_action("highlight", params);
	} catch (const confighandlerexception& e) {
		BOOST_CHECK(std::string(e.what()).length() > 0);
	}

	params[1] = "foo";
	params.push_back("bold");
	params.push_back("underline");
	try {
		rxman.handle_action("highlight", params);
	} catch (const confighandlerexception& e) {
		BOOST_CHECK_EQUAL(std::string(e.what()).length(), 0u);
	}

	params[0] = "all";
	try {
		rxman.handle_action("highlight", params);
	} catch (const confighandlerexception& e) {
		BOOST_CHECK_EQUAL(std::string(e.what()).length(), 0u);
	}

	try {
		rxman.handle_action("an-invalid-command", params);
	} catch (const confighandlerexception& e) {
		BOOST_CHECK(std::string(e.what()).length() > 0);
	}

	str = "<";
	rxman.quote_and_highlight(str, "feedlist");
	BOOST_CHECK_EQUAL(str, "<");

	str = "a<b>";
	rxman.quote_and_highlight(str, "feedlist");
	BOOST_CHECK_EQUAL(str, "a<b>");
}
BOOST_AUTO_TEST_CASE(TestHtmlRenderer) {
	htmlrenderer rnd(100);

	std::vector<std::string> lines;
	std::vector<linkpair> links;

	rnd.render("<a href=\"http://slashdot.org/\">slashdot</a>", lines, links, "");
	BOOST_CHECK(lines.size() >= 1);
	BOOST_CHECK_EQUAL(lines[0], "<u>slashdot</>[1]");
	BOOST_CHECK_EQUAL(links[0].first, "http://slashdot.org/");
	BOOST_CHECK_EQUAL(links[0].second, LINK_HREF);

	lines.erase(lines.begin(), lines.end());
	links.erase(links.begin(), links.end());

	rnd.render("hello<br />world!", lines, links, "");
	BOOST_CHECK(lines.size() >= 2);
	BOOST_CHECK_EQUAL(lines[0], "hello");
	BOOST_CHECK_EQUAL(lines[1], "world!");

	lines.erase(lines.begin(), lines.end());
	links.erase(links.begin(), links.end());

	rnd.render("3<sup>10</sup>", lines, links, "");
	BOOST_CHECK(lines.size() >= 1);
	BOOST_CHECK_EQUAL(lines[0], "3^10");

	lines.erase(lines.begin(), lines.end());
	links.erase(links.begin(), links.end());

	rnd.render("A<sub>i</sub>", lines, links, "");
	BOOST_CHECK(lines.size() >= 1);
	BOOST_CHECK_EQUAL(lines[0], "A[i]");
}

BOOST_AUTO_TEST_CASE(TestIndexPartitioning) {
	std::vector<std::pair<unsigned int, unsigned int> > partitions = utils::partition_indexes(0, 9, 2);
	BOOST_CHECK_EQUAL(partitions.size(), 2u);
	BOOST_CHECK_EQUAL(partitions[0].first, 0u);
	BOOST_CHECK_EQUAL(partitions[0].second, 4u);
	BOOST_CHECK_EQUAL(partitions[1].first, 5u);
	BOOST_CHECK_EQUAL(partitions[1].second, 9u);

	partitions = utils::partition_indexes(0, 10, 3);
	BOOST_CHECK_EQUAL(partitions.size(), 3u);
	BOOST_CHECK_EQUAL(partitions[0].first, 0u);
	BOOST_CHECK_EQUAL(partitions[0].second, 2u);
	BOOST_CHECK_EQUAL(partitions[1].first, 3u);
	BOOST_CHECK_EQUAL(partitions[1].second, 5u);
	BOOST_CHECK_EQUAL(partitions[2].first, 6u);
	BOOST_CHECK_EQUAL(partitions[2].second, 10u);

	partitions = utils::partition_indexes(0, 11, 3);
	BOOST_CHECK_EQUAL(partitions.size(), 3u);
	BOOST_CHECK_EQUAL(partitions[0].first, 0u);
	BOOST_CHECK_EQUAL(partitions[0].second, 3u);
	BOOST_CHECK_EQUAL(partitions[1].first, 4u);
	BOOST_CHECK_EQUAL(partitions[1].second, 7u);
	BOOST_CHECK_EQUAL(partitions[2].first, 8u);
	BOOST_CHECK_EQUAL(partitions[2].second, 11u);

	partitions = utils::partition_indexes(0, 199, 200);
	BOOST_CHECK_EQUAL(partitions.size(), 200u);

	partitions = utils::partition_indexes(0, 103, 1);
	BOOST_CHECK_EQUAL(partitions.size(), 1u);
	BOOST_CHECK_EQUAL(partitions[0].first, 0u);
	BOOST_CHECK_EQUAL(partitions[0].second, 103u);
}

BOOST_AUTO_TEST_CASE(TestCensorUrl) {
	BOOST_CHECK_EQUAL(utils::censor_url(""), "");
	BOOST_CHECK_EQUAL(utils::censor_url("foobar"), "foobar");
	BOOST_CHECK_EQUAL(utils::censor_url("foobar://xyz/"), "foobar://xyz/");

	BOOST_CHECK_EQUAL(utils::censor_url("http://newsbeuter.org/"), "http://newsbeuter.org/");
	BOOST_CHECK_EQUAL(utils::censor_url("https://newsbeuter.org/"), "https://newsbeuter.org/");

	BOOST_CHECK_EQUAL(utils::censor_url("http://@newsbeuter.org/"), "http://*:*@newsbeuter.org/");
	BOOST_CHECK_EQUAL(utils::censor_url("https://@newsbeuter.org/"), "https://*:*@newsbeuter.org/");

	BOOST_CHECK_EQUAL(utils::censor_url("http://foo:bar@newsbeuter.org/"), "http://*:*@newsbeuter.org/");
	BOOST_CHECK_EQUAL(utils::censor_url("https://foo:bar@newsbeuter.org/"), "https://*:*@newsbeuter.org/");

	BOOST_CHECK_EQUAL(utils::censor_url("http://aschas@newsbeuter.org/"), "http://*:*@newsbeuter.org/");
	BOOST_CHECK_EQUAL(utils::censor_url("https://aschas@newsbeuter.org/"), "https://*:*@newsbeuter.org/");

	BOOST_CHECK_EQUAL(utils::censor_url("xxx://aschas@newsbeuter.org/"), "xxx://*:*@newsbeuter.org/");

	BOOST_CHECK_EQUAL(utils::censor_url("http://foobar"), "http://foobar");
	BOOST_CHECK_EQUAL(utils::censor_url("https://foobar"), "https://foobar");

	BOOST_CHECK_EQUAL(utils::censor_url("http://aschas@host"), "http://*:*@host");
	BOOST_CHECK_EQUAL(utils::censor_url("https://aschas@host"), "https://*:*@host");
}

BOOST_AUTO_TEST_CASE(TestMakeAbsoluteUrl) {
	BOOST_CHECK_EQUAL(utils::absolute_url("http://foobar/hello/crook/", "bar.html"), "http://foobar/hello/crook/bar.html");
	BOOST_CHECK_EQUAL(utils::absolute_url("https://foobar/foo/", "/bar.html"), "https://foobar/bar.html");
	BOOST_CHECK_EQUAL(utils::absolute_url("https://foobar/foo/", "http://quux/bar.html"), "http://quux/bar.html");
	BOOST_CHECK_EQUAL(utils::absolute_url("http://foobar", "bla.html"), "http://foobar/bla.html");
	BOOST_CHECK_EQUAL(utils::absolute_url("http://test:test@foobar:33", "bla2.html"), "http://test:test@foobar:33/bla2.html");
}

BOOST_AUTO_TEST_CASE(TestBacktickEvaluation) {
	BOOST_CHECK_EQUAL(configparser::evaluate_backticks(""), "");
	BOOST_CHECK_EQUAL(configparser::evaluate_backticks("hello world"), "hello world");
	BOOST_CHECK_EQUAL(configparser::evaluate_backticks("foo`true`baz"), "foobaz");
	BOOST_CHECK_EQUAL(configparser::evaluate_backticks("foo`barbaz"), "foo`barbaz");
	BOOST_CHECK_EQUAL(configparser::evaluate_backticks("foo `true` baz"), "foo  baz");
	BOOST_CHECK_EQUAL(configparser::evaluate_backticks("foo `true` baz `xxx"), "foo  baz `xxx");
	BOOST_CHECK_EQUAL(configparser::evaluate_backticks("`echo hello world`"), "hello world");
	BOOST_CHECK_EQUAL(configparser::evaluate_backticks("xxx`echo yyy`zzz"), "xxxyyyzzz");
	BOOST_CHECK_EQUAL(configparser::evaluate_backticks("`echo 3 \\* 4 | bc`"), "12");
}

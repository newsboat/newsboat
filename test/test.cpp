/* test driver for newsbeuter */

#define BOOST_AUTO_TEST_MAIN
#include <boost/test/auto_unit_test.hpp>

#include <unistd.h>

#include <cache.h>
#include <rss.h>
#include <configcontainer.h>
#include <keymap.h>
#include <xmlpullparser.h>

#include <stdlib.h>

using namespace newsbeuter;

BOOST_AUTO_TEST_CASE(TestNewsbeuterReload) {
	configcontainer * cfg = new configcontainer();
	cache * rsscache = new cache("test-cache.db", cfg);

	rss_parser parser("http://bereshit.synflood.at/~ak/rss.xml", rsscache, cfg);
	rss_feed feed = parser.parse();
	BOOST_CHECK_EQUAL(feed.items().size(), 8u);

	rsscache->externalize_rssfeed(feed);
	rsscache->internalize_rssfeed(feed);
	BOOST_CHECK_EQUAL(feed.items().size(), 8u);

	BOOST_CHECK_EQUAL(feed.items()[0].title(), "Teh Saxxi");
	BOOST_CHECK_EQUAL(feed.items()[7].title(), "Handy als IR-Detektor");

	feed.items()[0].set_title("Another Title");
	feed.items()[0].set_pubDate(time(NULL));
	BOOST_CHECK_EQUAL(feed.items()[0].title(), "Another Title");

	rsscache->externalize_rssfeed(feed);

	rss_feed feed2(rsscache);
	feed2.set_rssurl("http://bereshit.synflood.at/~ak/rss.xml");
	rsscache->internalize_rssfeed(feed2);

	BOOST_CHECK_EQUAL(feed2.items().size(), 8u);
	BOOST_CHECK_EQUAL(feed2.items()[0].title(), "Another Title");
	BOOST_CHECK_EQUAL(feed2.items()[7].title(), "Handy als IR-Detektor");

	delete rsscache;
	delete cfg;

	::unlink("test-cache.db");
}

BOOST_AUTO_TEST_CASE(TestConfigParserAndContainer) {
	configcontainer * cfg = new configcontainer();
	configparser cfgparser("test-config.txt");
	cfg->register_commands(cfgparser);

	try {
		cfgparser.parse();
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
}

BOOST_AUTO_TEST_CASE(TestKeymap) {
	keymap k;
	BOOST_CHECK_EQUAL(k.get_operation("ENTER"), OP_OPEN);
	BOOST_CHECK_EQUAL(k.get_operation("CHAR(117)"), OP_SHOWURLS);
	BOOST_CHECK_EQUAL(k.get_operation("CHAR(88)"), OP_NIL);

	k.unset_key("enter");
	BOOST_CHECK_EQUAL(k.get_operation("ENTER"), OP_NIL);
	k.set_key(OP_OPEN, "enter");
	BOOST_CHECK_EQUAL(k.get_operation("ENTER"), OP_OPEN);

	BOOST_CHECK_EQUAL(k.get_opcode("open"), OP_OPEN);
	BOOST_CHECK_EQUAL(k.get_opcode("some-noexistent-operation"), OP_NIL);

	BOOST_CHECK_EQUAL(k.getkey(OP_OPEN), "enter");
	BOOST_CHECK_EQUAL(k.getkey(OP_TOGGLEITEMREAD), "N");
	BOOST_CHECK_EQUAL(k.getkey(static_cast<operation>(30000)), "<none>");

	BOOST_CHECK_EQUAL(k.get_key("CHAR(32)"), ' ');
	BOOST_CHECK_EQUAL(k.get_key("CHAR(85)"), 'U');
	BOOST_CHECK_EQUAL(k.get_key("CHAR(126)"), '~');
	BOOST_CHECK_EQUAL(k.get_key("INVALID"), 0);
}

BOOST_AUTO_TEST_CASE(TestXmlPullParser) {
	std::istringstream is("<test><foo quux='asdf' bar=\"qqq\">text</foo>more text<more>&quot;&#33;&#x40;</more></test>");
	xmlpullparser xpp;
	xmlpullparser::event e;
	xpp.setInput(is);

	e = xpp.getEventType();
	BOOST_CHECK_EQUAL(e, xmlpullparser::START_DOCUMENT);
	e = xpp.next();
	BOOST_CHECK_EQUAL(e, xmlpullparser::START_TAG);
	BOOST_CHECK_EQUAL(xpp.getText(), "test");
	BOOST_CHECK_EQUAL(xpp.getAttributeCount(), 0);
	e = xpp.next();
	BOOST_CHECK_EQUAL(e, xmlpullparser::START_TAG);
	BOOST_CHECK_EQUAL(xpp.getText(), "foo");
	BOOST_CHECK_EQUAL(xpp.getAttributeCount(), 2);
	BOOST_CHECK_EQUAL(xpp.getAttributeValue("quux"), "asdf");
	BOOST_CHECK_EQUAL(xpp.getAttributeValue("bar"), "qqq");
	e = xpp.next();
	BOOST_CHECK_EQUAL(e, xmlpullparser::TEXT);
	BOOST_CHECK_EQUAL(xpp.getText(), "text");
	BOOST_CHECK_EQUAL(xpp.getAttributeCount(), -1);
	e = xpp.next();
	BOOST_CHECK_EQUAL(e, xmlpullparser::END_TAG);
	BOOST_CHECK_EQUAL(xpp.getText(), "foo");
	BOOST_CHECK_EQUAL(xpp.getAttributeCount(), -1);
	e = xpp.next();
	BOOST_CHECK_EQUAL(e, xmlpullparser::TEXT);
	BOOST_CHECK_EQUAL(xpp.getText(), "more text");
	e = xpp.next();
	BOOST_CHECK_EQUAL(e, xmlpullparser::START_TAG);
	BOOST_CHECK_EQUAL(xpp.getText(), "more");
	e = xpp.next();
	BOOST_CHECK_EQUAL(e, xmlpullparser::TEXT);
	BOOST_CHECK_EQUAL(xpp.getText(), "\"!@");
	e = xpp.next();
	BOOST_CHECK_EQUAL(e, xmlpullparser::END_TAG);
	BOOST_CHECK_EQUAL(xpp.getText(), "more");
	e = xpp.next();
	BOOST_CHECK_EQUAL(e, xmlpullparser::END_TAG);
	BOOST_CHECK_EQUAL(xpp.getText(), "test");
	e = xpp.next();
	BOOST_CHECK_EQUAL(e, xmlpullparser::END_DOCUMENT);
	e = xpp.next();
	BOOST_CHECK_EQUAL(e, xmlpullparser::END_DOCUMENT);
}

/* test driver for rsspp */

#define BOOST_AUTO_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/auto_unit_test.hpp>

#include <rsspp.h>


BOOST_AUTO_TEST_CASE(TestParseSimpleRSS_0_91) {
	rsspp::parser p;

	rsspp::feed f = p.parse_file("data/rss091_1.xml");

	BOOST_CHECK_EQUAL(f.rss_version, rsspp::RSS_0_91);
	BOOST_CHECK_EQUAL(f.title, "Example Channel");
	BOOST_CHECK_EQUAL(f.description, "an example feed");
	BOOST_CHECK_EQUAL(f.link, "http://example.com/");

	BOOST_CHECK_EQUAL(f.items.size(), 1u);
	BOOST_CHECK_EQUAL(f.items[0].title, "1 < 2");
	BOOST_CHECK_EQUAL(f.items[0].link, "http://example.com/1_less_than_2.html");
	BOOST_CHECK_EQUAL(f.items[0].description, "1 < 2, 3 < 4.\nIn HTML, <b> starts a bold phrase\nand you start a link with <a href=");
	BOOST_CHECK_EQUAL(f.items[0].author, "");
	BOOST_CHECK_EQUAL(f.items[0].guid, "");
}

#define ENABLE_IMPLICIT_FILEPATH_CONVERSIONS

#include "itemlistformaction.h"

#include <fstream>
#include <tuple>
#include <unistd.h>

#include "3rd-party/catch.hpp"
#include "cache.h"
#include "configpaths.h"
#include "controller.h"
#include "itemlist.h"
#include "keymap.h"
#include "regexmanager.h"
#include "rssfeed.h"
#include "test_helpers/misc.h"
#include "test_helpers/tempfile.h"

using namespace newsboat;

TEST_CASE("OP_OPEN displays article using an external pager",
	"[ItemListFormAction]")
{
	ConfigPaths paths;
	Controller c(paths);
	newsboat::View v(c);
	test_helpers::TempFile pagerfile;

	const std::string test_url = "http://test_url";
	std::string test_title = "Article Title";
	std::string test_author = "Article Author";
	std::string test_description = "Article Description";
	time_t test_pubDate = 42;
	char test_pubDate_str[128];
	strftime(test_pubDate_str,
		sizeof(test_pubDate_str),
		"%a, %d %b %Y %H:%M:%S %z",
		localtime(&test_pubDate));

	ConfigContainer cfg;
	FilterContainer filters;
	RegexManager rxman;

	Cache rsscache(":memory:", cfg);
	cfg.set_configvalue("pager", "cat %f > " + pagerfile.get_path());

	std::shared_ptr<RssFeed> feed = std::make_shared<RssFeed>(&rsscache, "");

	std::shared_ptr<RssItem> item = std::make_shared<RssItem>(&rsscache);
	item->set_link(test_url);
	item->set_title(test_title);
	item->set_author(test_author);
	item->set_description(test_description, "text/plain");
	item->set_pubDate(test_pubDate);
	item->set_unread(true);
	feed->add_item(item);

	v.set_config_container(&cfg);
	c.set_view(&v);

	ItemListFormAction itemlist(v, itemlist_str, &rsscache, filters, &cfg, rxman);
	itemlist.set_feed(feed);

	const std::vector<std::string> args;
	REQUIRE_NOTHROW(itemlist.process_op(OP_OPEN, args));

	test_helpers::assert_article_file_content(pagerfile.get_path(),
		test_title,
		test_author,
		test_pubDate_str,
		test_url,
		test_description);
}

TEST_CASE("OP_PURGE_DELETED purges previously deleted items",
	"[ItemListFormAction]")
{
	ConfigPaths paths;
	Controller c(paths);
	newsboat::View v(c);
	ConfigContainer cfg;
	Cache rsscache(":memory:", cfg);
	FilterContainer filters;
	RegexManager rxman;
	std::shared_ptr<RssFeed> feed = std::make_shared<RssFeed>(&rsscache, "");
	std::shared_ptr<RssItem> item = std::make_shared<RssItem>(&rsscache);
	feed->add_item(item);

	v.set_config_container(&cfg);
	c.set_view(&v);

	ItemListFormAction itemlist(v, itemlist_str, &rsscache, filters, &cfg, rxman);
	itemlist.set_feed(feed);

	SECTION("No items to purge") {
		const std::vector<std::string> args;
		REQUIRE_NOTHROW(itemlist.process_op(OP_PURGE_DELETED, args));
		REQUIRE(feed->total_item_count() == 1);
	}

	SECTION("Deleted items are purged") {
		item->set_deleted(true);
		const std::vector<std::string> args;
		REQUIRE_NOTHROW(itemlist.process_op(OP_PURGE_DELETED, args));
		REQUIRE(feed->total_item_count() == 0);
	}
}

TEST_CASE(
	"OP_OPENBROWSER_AND_MARK passes the url to the browser and marks read",
	"[ItemListFormAction]")
{
	ConfigPaths paths;
	Controller c(paths);
	newsboat::View v(c);
	test_helpers::TempFile browserfile;

	const std::string test_url = "http://test_url";
	std::string line;

	ConfigContainer cfg;
	cfg.set_configvalue("browser", "echo %u >> " + browserfile.get_path());

	Cache rsscache(":memory:", cfg);
	FilterContainer filters;
	RegexManager rxman;

	std::shared_ptr<RssFeed> feed = std::make_shared<RssFeed>(&rsscache, "");
	std::shared_ptr<RssItem> item = std::make_shared<RssItem>(&rsscache);
	item->set_link(test_url);
	item->set_unread(true);
	feed->add_item(item);

	v.set_config_container(&cfg);
	c.set_view(&v);

	ItemListFormAction itemlist(v, itemlist_str, &rsscache, filters, &cfg, rxman);
	itemlist.set_feed(feed);
	const std::vector<std::string> args;
	itemlist.process_op(OP_OPENBROWSER_AND_MARK, args);
	std::ifstream browserFileStream(browserfile.get_path());

	REQUIRE(std::getline(browserFileStream, line));
	REQUIRE(line == test_url);

	REQUIRE(feed->unread_item_count() == 0);
}

TEST_CASE(
	"OP_OPENBROWSER_AND_MARK does not mark read when browser fails",
	"[ItemListFormAction]")
{
	ConfigPaths paths;
	Controller c(paths);
	newsboat::View v(c);

	const std::string test_url = "http://test_url";

	ConfigContainer cfg;
	cfg.set_configvalue("browser", "false %u");

	Cache rsscache(":memory:", cfg);
	FilterContainer filters;
	RegexManager rxman;

	std::shared_ptr<RssFeed> feed = std::make_shared<RssFeed>(&rsscache, "");
	std::shared_ptr<RssItem> item = std::make_shared<RssItem>(&rsscache);
	item->set_link(test_url);
	item->set_unread(true);
	feed->add_item(item);

	v.set_config_container(&cfg);
	c.set_view(&v);

	ItemListFormAction itemlist(v, itemlist_str, &rsscache, filters, &cfg, rxman);
	itemlist.set_feed(feed);
	const std::vector<std::string> args;
	itemlist.process_op(OP_OPENBROWSER_AND_MARK, args);

	REQUIRE(feed->unread_item_count() == 1);
}

TEST_CASE("OP_OPENINBROWSER passes the url to the browser",
	"[ItemListFormAction]")
{
	ConfigPaths paths;
	Controller c(paths);
	newsboat::View v(c);
	test_helpers::TempFile browserfile;
	const std::string test_url = "http://test_url";
	std::string line;

	ConfigContainer cfg;
	cfg.set_configvalue("browser", "echo %u >> " + browserfile.get_path());

	Cache rsscache(":memory:", cfg);
	FilterContainer filters;
	RegexManager rxman;

	std::shared_ptr<RssFeed> feed = std::make_shared<RssFeed>(&rsscache, "");
	std::shared_ptr<RssItem> item = std::make_shared<RssItem>(&rsscache);
	item->set_link(test_url);
	feed->add_item(item);

	v.set_config_container(&cfg);
	c.set_view(&v);

	ItemListFormAction itemlist(v, itemlist_str, &rsscache, filters, &cfg, rxman);
	itemlist.set_feed(feed);
	const std::vector<std::string> args;
	itemlist.process_op(OP_OPENINBROWSER, args);
	std::ifstream browserFileStream(browserfile.get_path());

	REQUIRE(std::getline(browserFileStream, line));
	REQUIRE(line == test_url);
}

TEST_CASE("OP_OPENINBROWSER_NONINTERACTIVE passes the url to the browser",
	"[ItemListFormAction]")
{
	ConfigPaths paths;
	Controller c(paths);
	newsboat::View v(c);
	test_helpers::TempFile browserfile;
	const std::string test_url = "http://test_url";
	std::string line;

	ConfigContainer cfg;
	cfg.set_configvalue("browser", "echo %u >> " + browserfile.get_path());

	Cache rsscache(":memory:", cfg);
	FilterContainer filters;
	RegexManager rxman;

	std::shared_ptr<RssFeed> feed = std::make_shared<RssFeed>(&rsscache, "");
	std::shared_ptr<RssItem> item = std::make_shared<RssItem>(&rsscache);
	item->set_link(test_url);
	feed->add_item(item);

	v.set_config_container(&cfg);
	c.set_view(&v);

	ItemListFormAction itemlist(v, itemlist_str, &rsscache, filters, &cfg, rxman);
	itemlist.set_feed(feed);
	const std::vector<std::string> args;
	itemlist.process_op(newsboat::OP_OPENINBROWSER_NONINTERACTIVE, args);
	std::ifstream browserFileStream(browserfile.get_path());

	REQUIRE(std::getline(browserFileStream, line));
	REQUIRE(line == test_url);
}

TEST_CASE("OP_OPENALLUNREADINBROWSER passes the url list to the browser",
	"[ItemListFormAction]")
{
	ConfigPaths paths;
	Controller c(paths);
	newsboat::View v(c);
	test_helpers::TempFile browserfile;
	std::unordered_set<std::string> url_set;
	const std::string test_url = "http://test_url";
	std::string line;
	int itemCount = 6;

	ConfigContainer cfg;
	cfg.set_configvalue("browser", "echo %u >> " + browserfile.get_path());

	Cache rsscache(":memory:", cfg);
	FilterContainer filters;
	RegexManager rxman;

	std::shared_ptr<RssFeed> feed = std::make_shared<RssFeed>(&rsscache, "");

	for (int i = 0; i < itemCount; i++) {
		std::shared_ptr<RssItem> item =
			std::make_shared<RssItem>(&rsscache);
		item->set_link(test_url + std::to_string(i));
		url_set.insert(test_url + std::to_string(i));
		item->set_unread(true);
		feed->add_item(item);
	}

	v.set_config_container(&cfg);
	c.set_view(&v);

	ItemListFormAction itemlist(v, itemlist_str, &rsscache, filters, &cfg, rxman);
	itemlist.set_feed(feed);

	SECTION("unread >= max-browser-tabs") {
		int maxItemsToOpen = 4;
		int openedItemsCount = 0;
		cfg.set_configvalue(
			"max-browser-tabs", std::to_string(maxItemsToOpen));

		const std::vector<std::string> args;
		itemlist.process_op(OP_OPENALLUNREADINBROWSER, args);

		std::ifstream browserFileStream(browserfile.get_path());
		openedItemsCount = 0;
		if (browserFileStream.is_open()) {
			while (std::getline(browserFileStream, line)) {
				INFO("Each URL should be present exactly once. "
					"Erase urls after first match to fail if "
					"an item opens twice.");
				REQUIRE(url_set.count(line) == 1);
				url_set.erase(url_set.find(line));
				openedItemsCount += 1;
			}
		}
		REQUIRE(openedItemsCount == maxItemsToOpen);
	}

	SECTION("unread < max-browser-tabs") {
		int maxItemsToOpen = 9;
		int openedItemsCount = 0;
		cfg.set_configvalue(
			"max-browser-tabs", std::to_string(maxItemsToOpen));

		const std::vector<std::string> args;
		itemlist.process_op(OP_OPENALLUNREADINBROWSER, args);

		std::ifstream browserFileStream(browserfile.get_path());
		if (browserFileStream.is_open()) {
			while (std::getline(browserFileStream, line)) {
				INFO("Each URL should be present exactly once. "
					"Erase urls after first match to fail if "
					"an item opens twice.");
				REQUIRE(url_set.count(line) == 1);
				url_set.erase(url_set.find(line));
				openedItemsCount += 1;
			}
		}
		REQUIRE(openedItemsCount == itemCount);
	}
}

TEST_CASE(
	"OP_OPENALLUNREADINBROWSER_AND_MARK passes the url list to the browser "
	"and marks them read",
	"[ItemListFormAction]")
{
	ConfigPaths paths;
	Controller c(paths);
	newsboat::View v(c);
	test_helpers::TempFile browserfile;
	std::unordered_set<std::string> url_set;
	const std::string test_url = "http://test_url";
	std::string line;
	const unsigned int itemCount = 6;

	ConfigContainer cfg;
	cfg.set_configvalue("browser", "echo %u >> " + browserfile.get_path());

	Cache rsscache(":memory:", cfg);
	FilterContainer filters;
	RegexManager rxman;

	std::shared_ptr<RssFeed> feed = std::make_shared<RssFeed>(&rsscache, "");

	for (unsigned int i = 0; i < itemCount; i++) {
		std::shared_ptr<RssItem> item =
			std::make_shared<RssItem>(&rsscache);
		item->set_link(test_url + std::to_string(i));
		url_set.insert(test_url + std::to_string(i));
		item->set_unread(true);
		feed->add_item(item);
	}

	v.set_config_container(&cfg);
	c.set_view(&v);

	ItemListFormAction itemlist(v, itemlist_str, &rsscache, filters, &cfg, rxman);
	itemlist.set_feed(feed);

	SECTION("unread >= max-browser-tabs") {
		const unsigned int maxItemsToOpen = 4;
		unsigned int openedItemsCount = 0;
		cfg.set_configvalue(
			"max-browser-tabs", std::to_string(maxItemsToOpen));

		const std::vector<std::string> args;
		itemlist.process_op(OP_OPENALLUNREADINBROWSER_AND_MARK, args);

		std::ifstream browserFileStream(browserfile.get_path());
		if (browserFileStream.is_open()) {
			while (std::getline(browserFileStream, line)) {
				INFO("Each URL should be present exactly once. "
					"Erase urls after first match to fail if "
					"an item opens twice.");
				REQUIRE(url_set.count(line) == 1);
				url_set.erase(url_set.find(line));
				openedItemsCount += 1;
			}
		}
		REQUIRE(openedItemsCount == maxItemsToOpen);
		REQUIRE(feed->unread_item_count() ==
			itemCount - maxItemsToOpen);
	}

	SECTION("unread < max-browser-tabs") {
		int maxItemsToOpen = 9;
		int openedItemsCount = 0;
		cfg.set_configvalue(
			"max-browser-tabs", std::to_string(maxItemsToOpen));

		const std::vector<std::string> args;
		itemlist.process_op(OP_OPENALLUNREADINBROWSER_AND_MARK, args);

		std::ifstream browserFileStream(browserfile.get_path());
		if (browserFileStream.is_open()) {
			while (std::getline(browserFileStream, line)) {
				INFO("Each URL should be present exactly once. "
					"Erase urls after first match to fail if "
					"an item opens twice.");
				REQUIRE(url_set.count(line) == 1);
				url_set.erase(url_set.find(line));
				openedItemsCount += 1;
			}
		}
		REQUIRE(openedItemsCount == itemCount);
		REQUIRE(feed->unread_item_count() == 0);
	}
}

TEST_CASE("OP_SHOWURLS shows the article's properties", "[ItemListFormAction]")
{
	ConfigPaths paths;
	Controller c(paths);
	newsboat::View v(c);
	ConfigContainer cfg;
	Cache rsscache(":memory:", cfg);
	FilterContainer filters;
	RegexManager rxman;
	test_helpers::TempFile urlFile;

	const std::string test_url = "http://test_url";
	std::string test_title = "Article Title";
	std::string test_author = "Article Author";
	std::string test_description = "Article Description";
	time_t test_pubDate = 42;
	char test_pubDate_str[128];
	strftime(test_pubDate_str,
		sizeof(test_pubDate_str),
		"%a, %d %b %Y %H:%M:%S %z",
		localtime(&test_pubDate));

	v.set_config_container(&cfg);
	c.set_view(&v);

	std::shared_ptr<RssFeed> feed = std::make_shared<RssFeed>(&rsscache, "");
	std::shared_ptr<RssItem> item = std::make_shared<RssItem>(&rsscache);

	item->set_link(test_url);
	item->set_title(test_title);
	item->set_author(test_author);
	item->set_description(test_description, "text/plain");
	item->set_pubDate(test_pubDate);
	ItemListFormAction itemlist(v, itemlist_str, &rsscache, filters, &cfg, rxman);

	SECTION("with external-url-viewer") {
		feed->add_item(item);
		itemlist.set_feed(feed);
		cfg.set_configvalue(
			"external-url-viewer", "tee > " + urlFile.get_path());

		const std::vector<std::string> args;
		REQUIRE_NOTHROW(itemlist.process_op(OP_SHOWURLS, args));

		test_helpers::assert_article_file_content(urlFile.get_path(),
			test_title,
			test_author,
			test_pubDate_str,
			test_url,
			test_description);
	}

	SECTION("internal url viewer") {
		feed->add_item(item);
		itemlist.set_feed(feed);
		const std::vector<std::string> args;
		REQUIRE_NOTHROW(itemlist.process_op(OP_SHOWURLS, args));
	}

	SECTION("no feed in formaction") {
		const std::vector<std::string> args;
		REQUIRE_NOTHROW(itemlist.process_op(OP_SHOWURLS, args));
	}
}

TEST_CASE("OP_BOOKMARK pipes articles url and title to bookmark-command",
	"[ItemListFormAction]")
{
	ConfigPaths paths;
	Controller c(paths);
	newsboat::View v(c);
	ConfigContainer cfg;
	Cache rsscache(":memory:", cfg);
	FilterContainer filters;
	RegexManager rxman;
	test_helpers::TempFile bookmarkFile;
	std::string line;
	std::vector<std::string> bookmark_args;

	const std::string test_url = "http://test_url";
	std::string test_title = "Article Title";
	std::string feed_title = "Feed Title";
	std::string separator = " ";
	std::string extra_arg = "extra arg";

	v.set_config_container(&cfg);
	c.set_view(&v);

	std::shared_ptr<RssFeed> feed = std::make_shared<RssFeed>(&rsscache, "");
	feed->set_title(feed_title);

	std::shared_ptr<RssItem> item = std::make_shared<RssItem>(&rsscache);
	item->set_link(test_url);
	item->set_title(test_title);

	ItemListFormAction itemlist(v, itemlist_str, &rsscache, filters, &cfg, rxman);

	feed->add_item(item);
	itemlist.set_feed(feed);

	cfg.set_configvalue(
		"bookmark-cmd", "echo > " + bookmarkFile.get_path());

	bookmark_args.push_back(extra_arg);

	auto checkOutput = [&] {
		std::ifstream browserFileStream(bookmarkFile.get_path());

		REQUIRE(std::getline(browserFileStream, line));
		REQUIRE(line ==
			test_url + separator + test_title + separator + extra_arg +
			separator + feed_title);
	};

	SECTION("Macro") {
		REQUIRE_NOTHROW(itemlist.process_op(OP_BOOKMARK, bookmark_args, BindingType::Macro));

		checkOutput();
	}

	SECTION("Bind") {
		REQUIRE_NOTHROW(itemlist.process_op(OP_BOOKMARK, bookmark_args, BindingType::Bind));

		checkOutput();
	}

}

TEST_CASE("OP_EDITFLAGS arguments are added to an item's flags",
	"[ItemListFormAction]")
{
	ConfigPaths paths;
	Controller c(paths);
	newsboat::View v(c);
	ConfigContainer cfg;
	Cache rsscache(":memory:", cfg);
	FilterContainer filters;
	RegexManager rxman;

	std::vector<std::string> op_args;

	v.set_config_container(&cfg);
	c.set_view(&v);

	std::shared_ptr<RssFeed> feed = std::make_shared<RssFeed>(&rsscache, "");
	std::shared_ptr<RssItem> item = std::make_shared<RssItem>(&rsscache);

	ItemListFormAction itemlist(v, itemlist_str, &rsscache, filters, &cfg, rxman);

	feed->add_item(item);
	itemlist.set_feed(feed);

	// <name, input, expected_result>
	std::vector<std::tuple<std::string, std::string, std::string>> tests {
		std::make_tuple("Single flag", "G", "G"),
		std::make_tuple("Unordered flags", "abdefc", "abcdef"),
		std::make_tuple("Duplicate flag in argument", "Abdddd", "Abd"),
		std::make_tuple("Unauthorized values in arguments: Numbers", "Abd1236", "Abd"),
		std::make_tuple("Unauthorized values in arguments: Symbolds", "Abd%^\\*;\'\"&~#{([-|`_/@)]=}$£€µ,;:!?./§", "Abd"),
		std::make_tuple("Unauthorized values in arguments: Accents", "Abd¨^", "Abd"),
		std::make_tuple("All possible flags at once", "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz", "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"),
	};

	SECTION("Macro") {
		for (const auto& testCase : tests) {
			DYNAMIC_SECTION("Test " << std::get<0>(testCase)) {
				std::vector<std::string> args = op_args;
				args.push_back(std::get<1>(testCase));
				REQUIRE_NOTHROW( itemlist.process_op(OP_EDITFLAGS, args, BindingType::Macro));
				REQUIRE(item->flags() == std::get<2>(testCase));
			}
		}
	}

	SECTION("Bind") {
		for (const auto& testCase : tests) {
			DYNAMIC_SECTION("Test " << std::get<0>(testCase)) {
				std::vector<std::string> args = op_args;
				args.push_back(std::get<1>(testCase));
				REQUIRE_NOTHROW( itemlist.process_op(OP_EDITFLAGS, args, BindingType::Bind));
				REQUIRE(item->flags() == std::get<2>(testCase));
			}
		}
	}
}

TEST_CASE("OP_SAVE writes an article's attributes to the specified file",
	"[ItemListFormAction]")
{
	ConfigPaths paths;
	Controller c(paths);
	newsboat::View v(c);
	test_helpers::TempFile saveFile;
	ConfigContainer cfg;
	Cache rsscache(":memory:", cfg);
	FilterContainer filters;
	RegexManager rxman;

	std::vector<std::string> op_args;
	op_args.push_back(saveFile.get_path());

	const std::string test_url = "http://test_url";
	std::string test_title = "Article Title";
	std::string test_author = "Article Author";
	std::string test_description = "Article Description";
	time_t test_pubDate = 42;
	char test_pubDate_str[128];
	strftime(test_pubDate_str,
		sizeof(test_pubDate_str),
		"%a, %d %b %Y %H:%M:%S %z",
		localtime(&test_pubDate));

	v.set_config_container(&cfg);
	c.set_view(&v);

	std::shared_ptr<RssFeed> feed = std::make_shared<RssFeed>(&rsscache, "");

	std::shared_ptr<RssItem> item = std::make_shared<RssItem>(&rsscache);
	item->set_link(test_url);
	item->set_title(test_title);
	item->set_author(test_author);
	item->set_pubDate(test_pubDate);
	item->set_description(test_description, "text/plain");

	ItemListFormAction itemlist(v, itemlist_str, &rsscache, filters, &cfg, rxman);

	feed->add_item(item);
	itemlist.set_feed(feed);

	auto check = [&] {
		test_helpers::assert_article_file_content(saveFile.get_path(),
			test_title,
			test_author,
			test_pubDate_str,
			test_url,
			test_description);
	};

	SECTION("Macro") {
		REQUIRE_NOTHROW(itemlist.process_op(OP_SAVE, op_args, BindingType::Macro));
		check();
	}

	SECTION("Bind") {
		REQUIRE_NOTHROW(itemlist.process_op(OP_SAVE, op_args, BindingType::Bind));
		check();
	}

}

TEST_CASE("OP_HELP command is processed", "[ItemListFormAction]")
{
	ConfigPaths paths;
	Controller c(paths);
	newsboat::View v(c);
	ConfigContainer cfg;
	Cache rsscache(":memory:", cfg);

	KeyMap k(KM_NEWSBOAT);
	v.set_keymap(&k);

	v.set_config_container(&cfg);
	c.set_view(&v);

	std::shared_ptr<RssFeed> feed = std::make_shared<RssFeed>(&rsscache, "");
	std::shared_ptr<RssItem> item = std::make_shared<RssItem>(&rsscache);

	feed->add_item(item);

	std::shared_ptr<ItemListFormAction> itemlist = v.push_itemlist(feed);

	const std::vector<std::string> args;
	REQUIRE_NOTHROW(itemlist->process_op(OP_HELP, args));
}

TEST_CASE("OP_HARDQUIT command is processed", "[ItemListFormAction]")
{
	ConfigPaths paths;
	Controller c(paths);
	newsboat::View v(c);
	ConfigContainer cfg;
	Cache rsscache(":memory:", cfg);
	FilterContainer filters;
	RegexManager rxman;

	KeyMap k(KM_NEWSBOAT);
	v.set_keymap(&k);

	v.set_config_container(&cfg);
	c.set_view(&v);

	std::shared_ptr<RssFeed> feed = std::make_shared<RssFeed>(&rsscache, "");

	ItemListFormAction itemlist(v, itemlist_str, &rsscache, filters, &cfg, rxman);
	itemlist.set_feed(feed);

	const std::vector<std::string> args;
	REQUIRE_NOTHROW(itemlist.process_op(OP_HARDQUIT, args));
}

TEST_CASE("Navigate back and forth using OP_NEXT and OP_PREV",
	"[ItemListFormAction]")
{
	// We are using the OP_SHOWURLS command to print the current
	// article'attibutes to a file, and assert the position was indeed
	// updated.
	ConfigPaths paths;
	Controller c(paths);
	test_helpers::TempFile articleFile;
	newsboat::View v(c);
	ConfigContainer cfg;
	cfg.set_configvalue(
		"external-url-viewer", "tee > " + articleFile.get_path());
	Cache rsscache(":memory:", cfg);
	std::string line;

	std::string first_article_title = "First_Article";
	std::string second_article_title = "Second_Article";
	std::string prefix_title = "Title: ";

	KeyMap k(KM_NEWSBOAT);
	v.set_keymap(&k);

	v.set_config_container(&cfg);
	c.set_view(&v);

	std::shared_ptr<RssFeed> feed = std::make_shared<RssFeed>(&rsscache, "");

	std::shared_ptr<RssItem> item = std::make_shared<RssItem>(&rsscache);
	item->set_title(first_article_title);
	feed->add_item(item);

	std::shared_ptr<RssItem> item2 = std::make_shared<RssItem>(&rsscache);
	item2->set_title(second_article_title);
	feed->add_item(item2);

	std::shared_ptr<ItemListFormAction> itemlist = v.push_itemlist(feed);

	const std::vector<std::string> args;
	REQUIRE_NOTHROW(itemlist->process_op(OP_NEXT, args));
	itemlist->process_op(OP_SHOWURLS, args);

	std::ifstream fileStream(articleFile.get_path());
	std::getline(fileStream, line);
	REQUIRE(line == prefix_title + second_article_title);

	REQUIRE_NOTHROW(itemlist->process_op(OP_PREV, args));
	itemlist->process_op(OP_SHOWURLS, args);

	fileStream.seekg(0);
	std::getline(fileStream, line);
	REQUIRE(line == prefix_title + first_article_title);
}

TEST_CASE("OP_TOGGLESHOWREAD switches the value of show-read-articles",
	"[ItemListFormAction]")
{
	ConfigPaths paths;
	Controller c(paths);
	newsboat::View v(c);
	ConfigContainer cfg;
	Cache rsscache(":memory:", cfg);

	KeyMap k(KM_NEWSBOAT);
	v.set_keymap(&k);

	v.set_config_container(&cfg);
	c.set_view(&v);

	std::shared_ptr<RssFeed> feed = std::make_shared<RssFeed>(&rsscache, "");

	std::shared_ptr<RssItem> item = std::make_shared<RssItem>(&rsscache);
	feed->add_item(item);

	std::shared_ptr<ItemListFormAction> itemlist = v.push_itemlist(feed);

	SECTION("True to False") {
		v.get_cfg()->set_configvalue("show-read-articles", "yes");
		const std::vector<std::string> args;
		REQUIRE_NOTHROW(itemlist->process_op(OP_TOGGLESHOWREAD, args));
		REQUIRE_FALSE(v.get_cfg()->get_configvalue_as_bool(
				"show-read-articles"));
	}
	SECTION("False to True") {
		v.get_cfg()->set_configvalue("show-read-articles", "no");
		const std::vector<std::string> args;
		REQUIRE_NOTHROW(itemlist->process_op(OP_TOGGLESHOWREAD, args));
		REQUIRE(v.get_cfg()->get_configvalue_as_bool(
				"show-read-articles"));
	}
}

TEST_CASE("OP_PIPE_TO pipes an article's content to an external command",
	"[ItemListFormAction]")
{
	ConfigPaths paths;
	Controller c(paths);
	newsboat::View v(c);
	test_helpers::TempFile articleFile;
	ConfigContainer cfg;
	Cache rsscache(":memory:", cfg);
	FilterContainer filters;
	RegexManager rxman;

	std::vector<std::string> op_args;
	op_args.push_back("tee > " + articleFile.get_path());

	const std::string test_url = "http://test_url";
	std::string test_title = "Article Title";
	std::string test_author = "Article Author";
	std::string test_description = "Article Description";
	time_t test_pubDate = 42;
	char test_pubDate_str[128];
	strftime(test_pubDate_str,
		sizeof(test_pubDate_str),
		"%a, %d %b %Y %H:%M:%S %z",
		localtime(&test_pubDate));

	v.set_config_container(&cfg);
	c.set_view(&v);

	std::shared_ptr<RssFeed> feed = std::make_shared<RssFeed>(&rsscache, "");

	std::shared_ptr<RssItem> item = std::make_shared<RssItem>(&rsscache);
	item->set_link(test_url);
	item->set_title(test_title);
	item->set_author(test_author);
	item->set_pubDate(test_pubDate);
	item->set_description(test_description, "text/plain");

	ItemListFormAction itemlist(v, itemlist_str, &rsscache, filters, &cfg, rxman);

	feed->add_item(item);
	itemlist.set_feed(feed);

	auto check = [&] {
		test_helpers::assert_article_file_content(articleFile.get_path(),
			test_title,
			test_author,
			test_pubDate_str,
			test_url,
			test_description);
	};

	SECTION("Macro") {
		REQUIRE_NOTHROW(itemlist.process_op(OP_PIPE_TO, op_args, BindingType::Macro));
		check();
	}

	SECTION("Bind") {
		REQUIRE_NOTHROW(itemlist.process_op(OP_PIPE_TO, op_args, BindingType::Bind));
		check();
	}
}

TEST_CASE("OP_OPENINBROWSER does not result in itemlist invalidation",
	"[ItemListFormAction]")
{
	ConfigPaths paths;
	Controller c(paths);
	ConfigContainer cfg;
	KeyMap k(KM_NEWSBOAT);
	newsboat::View v(c);
	v.set_config_container(&cfg);
	v.set_keymap(&k);
	Cache rsscache(":memory:", cfg);
	FilterContainer filters;
	RegexManager rxman;

	std::shared_ptr<RssItem> item1 = std::make_shared<RssItem>(&rsscache);
	item1->set_link("https://example.com/1");
	std::shared_ptr<RssItem> item2 = std::make_shared<RssItem>(&rsscache);
	item2->set_link("https://example.com/2");
	std::shared_ptr<RssItem> item3 = std::make_shared<RssItem>(&rsscache);
	item3->set_link("https://example.com/3");

	std::shared_ptr<RssFeed> feed = std::make_shared<RssFeed>(&rsscache, "");
	feed->add_item(item1);
	feed->add_item(item2);
	feed->add_item(item3);

	cfg.set_configvalue("mark-as-read-on-hover", "yes");
	cfg.set_configvalue("show-read-articles", "no");
	cfg.set_configvalue("browser", "echo");

	SECTION("by default, all items are marked as unread") {
		REQUIRE(item1->unread());
		REQUIRE(item2->unread());
		REQUIRE(item3->unread());
	}

	// The following sections have some interactions with STFL.
	// We call `Stfl::reset()` to make sure the terminal is in a regular mode
	// before calling Catch2 functions. Without the reset, Catch2 might output
	// text while the terminal is in application mode, which makes it invisble
	// when back in regular mode.
	// The `View` object calls `Stfl::reset()` in its destructor so we can be
	// sure we always return to the regular terminal mode, even when an
	// exception is thrown.

	SECTION("when entering ItemList, the first item is marked as 'read' due to 'mark-as-read-on-hover'") {
		auto itemlist = v.push_itemlist(feed);
		itemlist->prepare();

		Stfl::reset();
		REQUIRE_FALSE(item1->unread());
		REQUIRE(item2->unread());
		REQUIRE(item3->unread());
	}

	SECTION("executing 'open-in-browser' operation does not cause the next item to be marked as read") {
		auto itemlist = v.push_itemlist(feed);
		itemlist->prepare();

		const std::vector<std::string> args;
		bool success = itemlist->process_op(OP_OPENINBROWSER, args);
		Stfl::reset();
		REQUIRE(success);

		itemlist->prepare();
		Stfl::reset();

		// Verify that following regression is fixed:
		// https://github.com/newsboat/newsboat/issues/1292
		REQUIRE_FALSE(item1->unread());
		REQUIRE(item2->unread());
		REQUIRE(item3->unread());
	}
}

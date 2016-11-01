#include "catch.hpp"
#include "test-helpers.h"

#include <cache.h>
#include <itemlist_formaction.h>
#include <keymap.h>
#include <itemlist.h>

#include <unistd.h>

using namespace newsbeuter;

TEST_CASE("itemlist_formaction::process_op()") {
	controller c;
	newsbeuter::view v(&c);
	TestHelpers::TempFile browserfile;
	std::unordered_set<std::string> url_set;

	std::string test_url = "http://test_url";
	std::string line;
	int itemCount = 6;

	configcontainer cfg;
	cfg.set_configvalue("browser", "echo %u >> " + browserfile.getPath());

	cache rsscache(":memory:", &cfg);

	std::shared_ptr<rss_feed> feed(new rss_feed(&rsscache));

	for (int i = 0; i < itemCount; i++) {
		std::shared_ptr<rss_item> item = std::make_shared<rss_item>(&rsscache);
		item->set_link(test_url + std::to_string(i));
		url_set.insert(test_url + std::to_string(i));
		item->set_unread(true);
		feed->add_item(item);
	}

	v.set_config_container(&cfg);
	c.set_view(&v);

	itemlist_formaction itemlist(&v, itemlist_str);
	itemlist.set_feed(feed);

	SECTION("Itemlist Formaction"){
	
		SECTION("Open all unread in browser, unread >= max-browser-tabs"){
			int maxItemsToOpen = 4;
			int openedItemsCount = 0;
			cfg.set_configvalue("max-browser-tabs", std::to_string(maxItemsToOpen));

			itemlist.process_op(OP_OPENALLUNREADINBROWSER);

			std::ifstream browserFileStream (browserfile.getPath());
			openedItemsCount = 0;
			if (browserFileStream.is_open()) {
				for (int i = 0; i < maxItemsToOpen; i ++) {
					if ( std::getline (browserFileStream,line) ) {
						INFO("Each URL should be present exactly once. Erase urls after first match to fail if an item opens twice.")
						REQUIRE(url_set.count(test_url + std::to_string(i)) == 1);
						url_set.erase(url_set.find(test_url + std::to_string(i)));
						openedItemsCount += 1;
					}
				}
				browserFileStream.close();
			}
			REQUIRE(openedItemsCount == maxItemsToOpen);
		}

		SECTION("Open all unread in browser, unread < max-browser-tabs"){
			int maxItemsToOpen = 9;
			int openedItemsCount = 0;
			cfg.set_configvalue("max-browser-tabs", std::to_string(maxItemsToOpen));

			itemlist.process_op(OP_OPENALLUNREADINBROWSER);

			std::ifstream browserFileStream (browserfile.getPath());
			if (browserFileStream.is_open()) {
				for (int i = 0; i < itemCount; i ++) {
					if ( std::getline (browserFileStream,line) ) {
						INFO("Each URL should be present exactly once. Erase urls after first match to fail if an item opens twice.")
						REQUIRE(url_set.count(test_url + std::to_string(i)) == 1);
						url_set.erase(url_set.find(test_url + std::to_string(i)));
						openedItemsCount += 1;
					}
				}
				browserFileStream.close();
			}
			REQUIRE(openedItemsCount == itemCount);
		}

		SECTION("Open all unread in browser and mark read, unread >= max-browser-tabs"){
			int maxItemsToOpen = 4;
			int openedItemsCount = 0;
			cfg.set_configvalue("max-browser-tabs", std::to_string(maxItemsToOpen));

			itemlist.process_op(OP_OPENALLUNREADINBROWSER_AND_MARK);

			std::ifstream browserFileStream (browserfile.getPath());
			if (browserFileStream.is_open()) {
				for (int i = 0; i < itemCount; i ++) {
					if ( std::getline (browserFileStream,line) ) {
						INFO("Each URL should be present exactly once. Erase urls after first match to fail if an item opens twice.")
						REQUIRE(url_set.count(test_url + std::to_string(i)) == 1);
						url_set.erase(url_set.find(test_url + std::to_string(i)));
						openedItemsCount += 1;
					}
				}
				browserFileStream.close();
			}
			REQUIRE(openedItemsCount == maxItemsToOpen);
			REQUIRE(feed->unread_item_count() == itemCount - maxItemsToOpen);
		}

		SECTION("Open all unread in browser and mark read, unread < max-browser-tabs"){
			int maxItemsToOpen = 9;
			int openedItemsCount = 0;
			cfg.set_configvalue("max-browser-tabs", std::to_string(maxItemsToOpen));

			itemlist.process_op(OP_OPENALLUNREADINBROWSER_AND_MARK);

			std::ifstream browserFileStream (browserfile.getPath());
			if (browserFileStream.is_open()) {
				for (int i = 0; i < itemCount; i ++) {
					if ( std::getline (browserFileStream,line) ) {
						INFO("Each URL should be present exactly once. Erase urls after first match to fail if an item opens twice.")
						REQUIRE(url_set.count(test_url + std::to_string(i)) == 1);
						url_set.erase(url_set.find(test_url + std::to_string(i)));
						openedItemsCount += 1;
					}
				}
				browserFileStream.close();
			}
			REQUIRE(openedItemsCount == itemCount);
			REQUIRE(feed->unread_item_count() == 0);
		}
	}

}

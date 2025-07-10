#include "filtercontainer.h"

#include "3rd-party/catch.hpp"
#include "confighandlerexception.h"

using namespace Newsboat;

TEST_CASE("FilterContainer::handle_action handles `define-filter`",
	"[FilterContainer]")
{
	FilterContainer filters;

	const auto action = "define-filter";

	SECTION("Throws ConfigHandlerException if less than 2 parameters") {
		REQUIRE_THROWS_AS(filters.handle_action(action, {}), ConfigHandlerException);
		REQUIRE_THROWS_AS(filters.handle_action(action, {"param1"}),
			ConfigHandlerException);
	}

	SECTION("Throws ConfigHandlerException if second param can't be parsed as filter expression") {
		REQUIRE_THROWS_AS(filters.handle_action(action, {"filter1", "!?"}),
			ConfigHandlerException);
	}

	SECTION("Doesn't throw and increases size after correct `define-filter`") {
		REQUIRE(filters.size() == 0);
		REQUIRE_NOTHROW(filters.handle_action(action, {"filter1", "title = \"Updates\""}));
		REQUIRE(filters.size() == 1);
		REQUIRE_NOTHROW(filters.handle_action(action, {"filter2", "author =~ \"\\s*Doe$\""}));
		REQUIRE(filters.size() == 2);
	}
}

TEST_CASE("FilterContainer allows two filters to have the same name",
	"[FilterContainer]")
{
	FilterContainer filters;

	const auto action = "define-filter";
	const auto filter_name = "arbitrary name";

	REQUIRE_NOTHROW(filters.handle_action(action, {filter_name, "title = \"Updates\""}));
	REQUIRE_NOTHROW(filters.handle_action(action, {filter_name, "author != \"Jon Doe\""}));

	REQUIRE(filters.size() == 2);
}

TEST_CASE("FilterContainer allows the same filter to have two different names",
	"[FilterContainer]")
{
	FilterContainer filters;

	const auto action = "define-filter";
	const auto filter = "title = \"Whatever, really\"";

	REQUIRE_NOTHROW(filters.handle_action(action, {"first filter", filter}));
	REQUIRE_NOTHROW(filters.handle_action(action, {"second filter", filter}));

	REQUIRE(filters.size() == 2);
}

TEST_CASE("FilterContainer::handle_action throws ConfigHandlerException on unknown command",
	"[FilterContainer]")
{
	FilterContainer filters;

	REQUIRE_THROWS_AS(filters.handle_action("bind-key", {"ESC", "quit"}),
		ConfigHandlerException);
	REQUIRE_THROWS_AS(filters.handle_action("reset-unread-on-update", {"url1", "https://example.com/feed.xml"}),
		ConfigHandlerException);
}

TEST_CASE("FilterContainer::get_filters() gives direct access to stored filters",
	"[FilterContainer]")
{
	FilterContainer filters;

	const auto action = "define-filter";
	const auto name1 = "filter1";
	const auto filter1 =  "title == \"Best\"";
	const auto name2 = "other";
	const auto filter2 =  "author # \"Ted\"";

	filters.handle_action(action, {name1, filter1});
	filters.handle_action(action, {name2, filter2});

	REQUIRE(filters.get_filters().size() == 2);
	REQUIRE(filters.get_filters()[0].name == name1);
	REQUIRE(filters.get_filters()[0].expr == filter1);
	REQUIRE(filters.get_filters()[1].name == name2);
	REQUIRE(filters.get_filters()[1].expr == filter2);

	filters.get_filters().clear();
	REQUIRE(filters.size() == 0);
}

TEST_CASE("FilterContainer::dump_config() writes out all configured settings to the provided vector",
	"[FilterContainer]")
{
	FilterContainer filters;

	const auto action = "define-filter";

	filters.handle_action(action, {"first", "title = \"x\""});
	filters.handle_action(action, {"second", "author = \"Me\""});
	filters.handle_action(action, {"third", "content =~ \"Linux\""});

	std::vector<std::string> config;

	const auto comment =
		"# Comment to check that RssIgnores::dump_config() doesn't clear the vector";
	config.push_back(comment);

	filters.dump_config(config);

	REQUIRE(config.size() == 4); // three `define-filter`s plus one comment
	REQUIRE(config[0] == comment);
	REQUIRE(config[1] == R"#(define-filter "first" "title = \"x\"")#");
	REQUIRE(config[2] == R"#(define-filter "second" "author = \"Me\"")#");
	REQUIRE(config[3] == R"#(define-filter "third" "content =~ \"Linux\"")#");
}

TEST_CASE("FilterContainer::get_filter() returns non-value when filter does not exist",
	"[FilterContainer]")
{
	FilterContainer filters;

	const auto filter1_expression = R"(title = "title 1")";

	REQUIRE_FALSE(filters.get_filter("non-existing").has_value());
	REQUIRE_FALSE(filters.get_filter("filter1").has_value());

	REQUIRE_NOTHROW(filters.handle_action("define-filter", {"filter1", filter1_expression}));

	REQUIRE_FALSE(filters.get_filter("non-existing").has_value());
	REQUIRE(filters.get_filter("filter1").has_value());
	REQUIRE(filters.get_filter("filter1") == filter1_expression);
}

TEST_CASE("FilterContainer::get_filter() returns first filter with given name",
	"[FilterContainer]")
{
	FilterContainer filters;

	const auto action = "define-filter";
	const auto filter_name = "arbitrary name";

	const auto filter1 = R"(title = "title 1")";
	const auto filter2 = R"(title = "title 2")";
	const auto filter3 = R"(title = "title 3")";

	REQUIRE_NOTHROW(filters.handle_action(action, {"other filter", filter1}));
	REQUIRE_NOTHROW(filters.handle_action(action, {filter_name, filter2}));
	REQUIRE_NOTHROW(filters.handle_action(action, {filter_name, filter3}));

	REQUIRE(filters.size() == 3);

	REQUIRE(filters.get_filter(filter_name) == filter2);
}

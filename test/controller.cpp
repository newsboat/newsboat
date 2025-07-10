#include "controller.h"

#include <string>
#include <unistd.h>

#include "configpaths.h"
#include "cache.h"
#include "utils.h"
#include "rssitem.h"
#include "3rd-party/catch.hpp"
#include "test_helpers/tempdir.h"
#include "test_helpers/envvar.h"

using namespace Newsboat;

TEST_CASE("write_item correctly parses path", "[Controller]")
{
	std::string name = "myitem";
	test_helpers::TempDir tmp;
	const auto home_dir = tmp.get_path() + "home/";
	REQUIRE(0 == utils::mkdir_parents(home_dir, 0700));
	const auto save_path = tmp.get_path() + "save/";
	REQUIRE(0 == utils::mkdir_parents(save_path, 0700));

	test_helpers::EnvVar home("HOME");
	home.set(home_dir);

	ConfigPaths paths{};
	Controller c(paths);

	auto cfg = c.get_config();
	cfg->set_configvalue("save-path", save_path);
	Cache rsscache(":memory:", cfg);


	RssItem item(&rsscache);
	item.set_title("title");
	const auto description = "First line.\nSecond one.\nAnd finally the third";
	item.set_description(description, "text/plain");

	c.write_item(item, tmp.get_path() + name);
	c.write_item(item, "~/" + name);
	c.write_item(item, name);

	REQUIRE(::access(std::string(tmp.get_path() + name).c_str(), R_OK) == 0);
	REQUIRE(::access(std::string(home_dir + name).c_str(), R_OK) == 0);
	REQUIRE(::access(std::string(save_path + name).c_str(), R_OK) == 0);
}

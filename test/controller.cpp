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
#include "test_helpers/misc.h"

using namespace newsboat;

TEST_CASE("write_item correctly parses path", "[Controller]")
{
	const auto name = Filepath::from_locale_string("myitem");
	test_helpers::TempDir tmp;
	const auto home_dir = tmp.get_path().join(Filepath::from_locale_string("home"));
	REQUIRE(0 == utils::mkdir_parents(home_dir, 0700));
	const auto save_path = tmp.get_path().join(Filepath::from_locale_string("save"));
	REQUIRE(0 == utils::mkdir_parents(save_path, 0700));

	test_helpers::EnvVar home("HOME");
	home.set(home_dir.to_locale_string());

	ConfigPaths paths{};
	Controller c(paths);

	auto cfg = c.get_config();
	cfg->set_configvalue("save-path", save_path.to_locale_string());
	auto rsscache = Cache::in_memory(*cfg);

	RssItem item(rsscache.get());
	item.set_title("title");
	const auto description = "First line.\nSecond one.\nAnd finally the third";
	item.set_description(description, "text/plain");

	c.write_item(item, tmp.get_path().join(name));
	c.write_item(item, Filepath::from_locale_string("~").join(name));
	c.write_item(item, name);

	REQUIRE(test_helpers::file_available_for_reading(tmp.get_path().join(name)));
	REQUIRE(test_helpers::file_available_for_reading(home_dir.join(name)));
	REQUIRE(test_helpers::file_available_for_reading(save_path.join(name)));
}

#include "view.h"

#include <string>

#include "3rd-party/catch.hpp"
#include "controller.h"
#include "configpaths.h"
#include "cache.h"

using namespace Newsboat;

TEST_CASE("get_filename_suggestion() normalizes filenames for saving articles", "[View]")
{

	const std::string example_ru("Инженеры из MIT");
	const std::string example_fr("Les mathématiques");

	const std::string example_en("Comparing Visual");

	ConfigPaths paths{};
	Controller c(paths);
	Newsboat::View v(c);

	ConfigContainer cfg{};
	Cache rsscache(":memory:", &cfg);

	v.set_config_container(&cfg);
	c.set_view(&v);

	// Default case is exclusively ASCII characters. Should never fail.
	REQUIRE(v.get_filename_suggestion(example_en).compare("Comparing_Visual.txt") ==
		0);

	REQUIRE(v.get_filename_suggestion(
			example_ru).compare("Инженеры_из_MIT.txt") != 0);
	REQUIRE(v.get_filename_suggestion(example_fr).compare("Les_mathématiques.txt") != 0);

	cfg.toggle("restrict-filename");

	REQUIRE(v.get_filename_suggestion(
			example_ru).compare("Инженеры из MIT.txt") == 0);
	REQUIRE(v.get_filename_suggestion(example_fr).compare("Les mathématiques.txt") == 0);
}

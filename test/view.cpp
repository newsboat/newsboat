#include "view.h"

#include <string>

#include "3rd-party/catch.hpp"
#include "controller.h"
#include "configpaths.h"
#include "cache.h"
#include "filepath.h"

using namespace newsboat;

TEST_CASE("get_filename_suggestion() normalizes filenames for saving articles", "[View]")
{

	const std::string example_ru("Инженеры из MIT");
	const std::string example_fr("Les mathématiques");

	const std::string example_en("Comparing Visual");

	ConfigPaths paths{};
	Controller c(paths);
	newsboat::View v(c);

	ConfigContainer cfg{};
	Cache rsscache(":memory:", &cfg);

	v.set_config_container(&cfg);
	c.set_view(&v);

	// Default case is exclusively ASCII characters. Should never fail.
	REQUIRE(v.get_filename_suggestion(example_en) ==
		Filepath::from_locale_string("Comparing_Visual.txt"));
	REQUIRE(v.get_filename_suggestion(example_ru) !=
		Filepath::from_locale_string("Инженеры_из_MIT.txt"));
	REQUIRE(v.get_filename_suggestion(example_fr) !=
		Filepath::from_locale_string("Les_mathématiques.txt"));

	cfg.toggle("restrict-filename");

	REQUIRE(v.get_filename_suggestion(example_ru) ==
		Filepath::from_locale_string("Инженеры из MIT.txt"));
	REQUIRE(v.get_filename_suggestion(example_fr) ==
		Filepath::from_locale_string("Les mathématiques.txt"));
}

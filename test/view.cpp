#include "view.h"

#include <string>

#include "3rd-party/catch.hpp"
#include "controller.h"
#include "configpaths.h"
#include "cache.h"

using namespace newsboat;

TEST_CASE("get_filename_suggestion() normalizes filenames for saving articles", "[View]") {

    std::string example_ru("Инженеры из MIT придумали остроумный способ очистки металлов от соли и грязи");
    std::string example_fr("Les mathématiques ont-elles pris le pouvoir sur le réel ?");

    std::string example_en("Comparing Visual Reasoning in Humans and AI");
    std::string example_en_default("Comparing_Visual_Reasoning_in_Humans_and_AI.txt");

    ConfigPaths paths{};
    Controller c(paths);
    newsboat::View v(&c);

    ConfigContainer cfg{};
    Cache rsscache(":memory:", &cfg);

    v.set_config_container(&cfg);
    c.set_view(&v);

    // Default case is exclusively ASCII characters. Should never fail.
    REQUIRE(v.get_filename_suggestion(example_en).compare(example_en_default) == 0);

    REQUIRE(v.get_filename_suggestion(example_ru).compare(example_ru.append(".txt")) != 0);
    REQUIRE(v.get_filename_suggestion(example_fr).compare(example_fr.append(".txt")) != 0);

    cfg.toggle("restrict-filename");

    REQUIRE(v.get_filename_suggestion(example_ru).compare(example_ru.append(".txt")) == 0);
    REQUIRE(v.get_filename_suggestion(example_fr).compare(example_fr.append(".txt")) == 0);
}
#include "charencoding.h"

#include <map>

#include "3rd-party/catch.hpp"

using namespace Newsboat;

TEST_CASE("charset_from_bom", "[charencoding]")
{
	const std::map<std::string, std::optional<std::string>> test_cases {
		{ "", std::nullopt },
		{ "text without BOM", std::nullopt },
		{ "\xEF\xBB\xBFtext with BOM", "UTF-8" },
		{ "\xFE\xFFtext with BOM", "UTF-16BE" },
		{ "\xFF\xFEtext with BOM", "UTF-16LE" },
	};

	for (const auto& test_case : test_cases) {
		std::vector<std::uint8_t> input(test_case.first.begin(), test_case.first.end());

		const auto actual = charencoding::charset_from_bom(input);
		const auto expected = test_case.second;

		INFO("actual: " << (actual.has_value() ? actual.value().c_str() : ""));
		INFO("expected: " << (expected.has_value() ? expected.value().c_str() : ""));

		REQUIRE(actual == expected);
	}
}

TEST_CASE("charset_from_xml_declaration", "[charencoding]")
{
	const std::map<std::string, std::optional<std::string>> test_cases {
		{ "", std::nullopt },
		{ "not a declaration", std::nullopt },
		{ R"(<?xml version="1.0"?>No encoding specified)", std::nullopt },
		{ R"(<?xml version="1.0" encoding="UTF-8"?>Encoding specified)", "UTF-8" },
		{ R"(<?xml version="1.0" encoding="utf-8"?>Encoding specified)", "utf-8" },
		{ R"(<?xml version="1.0" encoding="fake.encoding"?>Encoding specified)", "fake.encoding" },
	};

	for (const auto& test_case : test_cases) {
		std::vector<std::uint8_t> input(test_case.first.begin(), test_case.first.end());

		const auto actual = charencoding::charset_from_xml_declaration(input);
		const auto expected = test_case.second;

		INFO("actual: " << (actual.has_value() ? actual.value().c_str() : ""));
		INFO("expected: " << (expected.has_value() ? expected.value().c_str() : ""));

		REQUIRE(actual == expected);
	}
}

TEST_CASE("charset_from_content_type_header", "[charencoding]")
{
	const std::map<std::string, std::optional<std::string>> test_cases {
		{ "", std::nullopt },
		{ "application/xml", std::nullopt },
		{ "multipart/form-data; boundary=something", std::nullopt },
		{ "application/xml; charset=utf-8", "utf-8" },
	};

	for (const auto& test_case : test_cases) {
		std::vector<std::uint8_t> input(test_case.first.begin(), test_case.first.end());

		const auto actual = charencoding::charset_from_content_type_header(input);
		const auto expected = test_case.second;

		INFO("input: " << test_case.first);
		INFO("actual: " << (actual.has_value() ? actual.value().c_str() : ""));
		INFO("expected: " << (expected.has_value() ? expected.value().c_str() : ""));

		REQUIRE(actual == expected);
	}
}

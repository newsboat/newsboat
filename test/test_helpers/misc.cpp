#include "misc.h"

#include <fstream>
#include <unistd.h>

#include "3rd-party/catch.hpp"

void test_helpers::assert_article_file_content(const newsboat::Filepath& path,
	const std::string& title,
	const std::string& author,
	const std::string& date,
	const std::string& url,
	const std::string& description)
{
	std::string prefix_title = "Title: ";
	std::string prefix_author = "Author: ";
	std::string prefix_date = "Date: ";
	std::string prefix_link = "Link: ";

	std::string line;
	std::ifstream articleFileStream(path);
	REQUIRE(std::getline(articleFileStream, line));
	REQUIRE(line == prefix_title + title);

	REQUIRE(std::getline(articleFileStream, line));
	REQUIRE(line == prefix_author + author);

	REQUIRE(std::getline(articleFileStream, line));
	REQUIRE(line == prefix_date + date);

	REQUIRE(std::getline(articleFileStream, line));
	REQUIRE(line == prefix_link + url);

	REQUIRE(std::getline(articleFileStream, line));
	REQUIRE(line == " ");

	REQUIRE(std::getline(articleFileStream, line));
	REQUIRE(line == description);

	REQUIRE(std::getline(articleFileStream, line));
	REQUIRE(line == "");
};

void test_helpers::copy_file(const newsboat::Filepath& source,
	const newsboat::Filepath& destination)
{
	std::ifstream  src(source, std::ios::binary);
	std::ofstream  dst(destination, std::ios::binary);

	REQUIRE(src.is_open());
	REQUIRE(dst.is_open());

	dst << src.rdbuf();
}

std::vector<std::string> test_helpers::file_contents(const newsboat::Filepath& filepath)
{
	std::vector<std::string> lines;

	std::ifstream in(filepath);
	while (in.is_open() && !in.eof()) {
		std::string line;
		std::getline(in, line);
		lines.emplace_back(std::move(line));
	}

	return lines;
}

bool test_helpers::starts_with(const std::string& prefix,
	const std::string& input)
{
	return input.substr(0, prefix.size()) == prefix;
}

bool test_helpers::ends_with(const std::string& suffix,
	const std::string& input)
{
	if (input.size() < suffix.size()) {
		return false;
	} else {
		return input.substr(input.size() - suffix.size(), suffix.size()) == suffix;
	}
}

bool test_helpers::file_exists(const newsboat::Filepath& filepath)
{
	return access(filepath.to_locale_string().c_str(), F_OK) == 0;
}

TEST_CASE("ends_with", "[test_helpers]")
{
	REQUIRE_FALSE(test_helpers::ends_with("bye", "hello"));
	REQUIRE_FALSE(test_helpers::ends_with("bye", "hi"));

	REQUIRE(test_helpers::ends_with("ype", "type"));
	REQUIRE(test_helpers::ends_with("pe", "type"));
	REQUIRE(test_helpers::ends_with("e", "type"));
	REQUIRE(test_helpers::ends_with("", "type"));
}

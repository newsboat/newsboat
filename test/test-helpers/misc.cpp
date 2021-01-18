#include "misc.h"

#include <fstream>
#include <unistd.h>

#include "3rd-party/catch.hpp"

void TestHelpers::assert_article_file_content(const std::string& path,
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

void TestHelpers::copy_file(const std::string& source,
	const std::string& destination)
{
	std::ifstream  src(source, std::ios::binary);
	std::ofstream  dst(destination, std::ios::binary);

	REQUIRE(src.is_open());
	REQUIRE(dst.is_open());

	dst << src.rdbuf();
}

std::vector<std::string> TestHelpers::file_contents(const std::string& filepath)
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

bool TestHelpers::starts_with(const std::string& input,
	const std::string& prefix)
{
	return input.substr(0, prefix.size()) == prefix;
}

bool TestHelpers::file_exists(const std::string& filepath)
{
	return access(filepath.c_str(), F_OK) == 0;
}

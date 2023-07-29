#include "misc.h"

#include <fstream>
#include <unistd.h>
#include <sys/stat.h>

#include "3rd-party/catch.hpp"

void test_helpers::assert_article_file_content(const std::string& path,
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

void test_helpers::copy_file(const std::string& source,
	const std::string& destination)
{
	std::ifstream  src(source, std::ios::binary);
	std::ofstream  dst(destination, std::ios::binary);

	REQUIRE(src.is_open());
	REQUIRE(dst.is_open());

	dst << src.rdbuf();
}

std::vector<std::string> test_helpers::file_contents(const std::string& filepath)
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

std::vector<std::uint8_t> test_helpers::read_binary_file(const std::string& filepath)
{
	std::ifstream file(filepath, std::ios::binary | std::ios::ate);
	std::streampos length = file.tellg();
	file.seekg(0, std::ios::beg);

	std::vector<std::uint8_t> buffer(length);
	file.read(reinterpret_cast<char*>(buffer.data()), length);
	return buffer;
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

bool test_helpers::file_exists(const std::string& filepath)
{
	return access(filepath.c_str(), F_OK) == 0;
}

int test_helpers::mkdir(const newsboat::Filepath& dirpath, mode_t mode)
{
	const auto dirpath_str = dirpath.to_locale_string();
	return ::mkdir(dirpath_str.c_str(), mode);
}

bool test_helpers::file_available_for_reading(const newsboat::Filepath& filepath)
{
	const auto filepath_str = filepath.to_locale_string();
	return (0 == ::access(filepath_str.c_str(), R_OK));
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

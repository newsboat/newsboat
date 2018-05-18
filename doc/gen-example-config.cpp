#include <iostream>
#include <sstream>

#include "split.h"

std::string to80Columns(std::string input)
{
	std::istringstream s(input);
	const std::string prefix = "#";
	const int limit = 80;
	std::string result;
	std::string curline = prefix;

	std::string word;
	while (s >> word) {
		if (curline.length() + 1 + word.length() < limit) {
			curline += ' ' + word;
		} else {
			result += curline + '\n';
			curline = prefix + ' ' + word;
		}
	}

	result += curline + '\n';

	return result;
}

int main()
{
	std::cout << "#\n";
	std::cout << "# Newsboat's example config\n";
	std::cout << "#\n\n";

	for (std::string line; std::getline(std::cin, line);) {
		const std::vector<std::string> matches = split(line, "||");
		if (matches.size() == 5) {
			const std::string option = matches[0];
			const std::string syntax = matches[1];
			const std::string defaultparam = matches[2];
			const std::string desc = matches[3];
			const std::string example = matches[4];

			std::cout << "####  " << option << '\n';
			std::cout << "#\n";
			std::cout << to80Columns(std::move(desc));
			std::cout << "#\n";
			std::cout << "# Syntax: " << syntax << '\n';
			std::cout << "#\n";
			std::cout << "# Default value: " << defaultparam
				  << '\n';
			std::cout << "#\n";
			std::cout << "# " << example << '\n';
			std::cout << '\n';
		}
	}

	return 0;
}

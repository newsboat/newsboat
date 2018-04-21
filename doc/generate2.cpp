#include <fstream>
#include <iostream>

#include "split.h"

int main(int argc, char* argv[])
{
	if (argc < 2) {
		std::cerr << "usage: " << argv[0]
			  << " <dsv-file> [<link-prefix>]\n";
		return 1;
	}

	std::ifstream input(argv[1]);
	if (!input.is_open()) {
		std::cerr << "couldn't open " << argv[1] << '\n';
		return 1;
	}

	int lineno = 0;
	for (std::string line; std::getline(input, line);) {
		++lineno;
		const std::vector<std::string> matches = split(line, "||");
		if (matches.size() == 3) {
			const std::string cmd = matches[0];
			const std::string key = matches[1];
			const std::string desc = matches[2];

			std::cout << "'" << cmd << "' ";
			std::cout << "(default key: '" << key << "')::\n";
			std::cout << "         " << desc << "\n\n";
		} else {
			std::cerr << "expected exactly 3 cells in " << argv[1]
				  << ":" << lineno;
			std::cerr << ", but got " << matches.size()
				  << " instead\n";
			return 1;
		}
	}

	return 0;
}

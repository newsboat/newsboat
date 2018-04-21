#include <iostream>
#include <fstream>

#include "split.h"

int main(int argc, char *argv[])
{
	if (argc < 2) {
		std::cerr << "usage: " << argv[0] << " <dsv-file> [<link-prefix>]\n";
		return 1;
	}

	std::ifstream input(argv[1]);
	if (! input.is_open()) {
		std::cerr << "couldn't open " << argv[1] << '\n';
		return 1;
	}

	const std::string linkprefix = argc == 3 ? argv[2] : "";

	int lineno = 0;
	for (std::string line; std::getline(input, line); ) {
		++lineno;
		const std::vector<std::string> matches = split(line, "||");
		if (matches.size() == 5) {
			const std::string option = matches[0];
			const std::string syntax = matches[1];
			const std::string defaultparam = matches[2];
			const std::string desc = matches[3];
			const std::string example = matches[4];

			std::cout << "[[" << linkprefix << option << "]] '" << option << "' ";
			std::cout << "(parameters: " << syntax << "; ";
			std::cout << "default value: '" << defaultparam << "')::\n";
			std::cout << "         " << desc;
			std::cout << " (example: " << example <<  ")\n\n";
		} else {
			std::cerr << "expected exactly 5 cells in " << argv[1] << ":" << lineno;
			std::cerr << ", but got " << matches.size() << " instead\n";
			return 1;
		}
	}

	return 0;
}

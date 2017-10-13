#include <iostream>
#include <regex>
#include <fstream>

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

	for (std::string line; std::getline(input, line); ) {
		const std::regex rgx(
				"([^:]*):([^:]*):(.*)",
				std::regex_constants::awk);
		std::smatch matches;
		if (std::regex_match(line, matches, rgx)) {
			if (matches.size() == 4) {
				const std::string cmd = matches[1].str();
				const std::string key = matches[2].str();
				const std::string desc = matches[3].str();

				std::cout <<  "'" << cmd << "' ";
				std::cout << "(default key: '" << key << "')::\n";
				std::cout << "         " << desc << "\n\n";
			}
		}
	}

	return 0;
}

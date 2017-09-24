#include <iostream>
#include <regex>
#include <fstream>

int main(int argc, char *argv[])
{
	if (argc < 2) {
		std::cerr << "usage: " << argv[0] << " <dsv-file>\n";
		return 1;
	}

	std::ifstream input(argv[1]);
	if (! input.is_open()) {
		std::cerr << "couldn't open " << argv[1] << '\n';
		return 1;
	}

	for (std::string line; std::getline(input, line); ) {
		const std::regex rgx(
				"([^|]*)\\|([^|]*)\\|([^|]*)\\|([^|]*)\\|(.*)",
				std::regex_constants::awk);
		std::smatch matches;
		if (std::regex_match(line, matches, rgx)) {
			if (matches.size() == 6) {
				const std::string option = matches[1].str();
				const std::string syntax = matches[2].str();
				const std::string defaultparam = matches[3].str();
				const std::string desc = matches[4].str();
				const std::string example = matches[5].str();

				std::cout << "'" << option << "' ";
				std::cout << "(parameters: " << syntax << "; ";
				std::cout << "default value: '" << defaultparam << "')::\n";
				std::cout << "         " << desc;
				std::cout << " (example: " << example <<  ")\n\n";
			}
		}
	}

	return 0;
}

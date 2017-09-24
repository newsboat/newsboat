#include <iostream>
#include <regex>
#include <sstream>

std::string to80Columns(std::string input) {
	std::istringstream s(input);
	const std::string prefix  = "#";
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

	for (std::string line; std::getline(std::cin, line); ) {
		const std::regex rgx(
				"([^|]*)\\|([^|]*)\\|([^|]*)\\|([^|]*)\\|(.*)",
				std::regex_constants::awk);
		std::smatch matches;
		if (std::regex_match(line, matches, rgx)) {
			if (matches.size() == 6) {
				const std::string option = matches[1].str();
				const std::string syntax = matches[2].str();
				std::string defaultparam = matches[3].str();
				std::string desc = matches[4].str();
				const std::string example = matches[5].str();

				const std::regex limitation("limitation in AsciiDoc");
				if (std::regex_search(desc, limitation)) {
					const std::regex parens(" \\([^)]*\\)\\.$");
					desc = std::regex_replace(desc, parens, ".");

					defaultparam = std::regex_replace(
							defaultparam, std::regex(";"), "|");
				}

				std::cout << "####  " << option << '\n';
				std::cout << "#\n";
				std::cout << to80Columns(std::move(desc));
				std::cout << "#\n";
				std::cout << "# Syntax: " << syntax << '\n';
				std::cout << "#\n";
				std::cout << "# Default value: " << defaultparam << '\n';
				std::cout << "#\n";
				std::cout << "# " << example << '\n';
				std::cout << '\n';
			}
		}
	}

	return 0;
}

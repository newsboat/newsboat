#include <vector>

std::vector<std::string> split(const std::string& str, const std::string& delim)
{
	std::vector<std::string> tokens;
	size_t prev = 0;
	do {
		size_t pos = str.find(delim, prev);
		if (pos == std::string::npos) {
			pos = str.length();
		}
		tokens.push_back(str.substr(prev, pos - prev));
		prev = pos + delim.length();
	} while (prev < str.length());
	return tokens;
}

#include "regexowner.h"

#include <vector>

namespace Newsboat {

Regex::Regex(const regex_t& r)
	: regex(r)
{
}

Regex::~Regex()
{
	regfree(&regex);
}

std::unique_ptr<Regex> Regex::compile(const std::string& reg_expression,
	int regcomp_flags, std::string& error)
{
	regex_t regex;
	const int err = regcomp(&regex, reg_expression.c_str(), regcomp_flags);
	if (err != 0) {
		size_t errLength = regerror(err, &regex, nullptr, 0);
		std::vector<char> buf(errLength);
		regerror(err, &regex, buf.data(), errLength);
		buf.pop_back(); // Remove trailing '\0'
		error.assign(buf.begin(), buf.end());
		return nullptr;
	}
	return std::unique_ptr<Regex>(new Regex(regex));
}

std::vector<std::pair<int, int>> Regex::matches(const std::string& input,
		int max_matches, int flags) const
{
	std::vector<regmatch_t> regMatches(max_matches);
	if (regexec(&regex, input.c_str(), max_matches,
			regMatches.data(), flags) == 0) {
		std::vector<std::pair<int, int>> results;
		for (const auto& regMatch : regMatches) {
			if (regMatch.rm_so < 0 || regMatch.rm_eo < 0) {
				break;
			}
			results.push_back({regMatch.rm_so, regMatch.rm_eo});
		}
		return results;
	}
	return {};
}

} // namespace Newsboat

#ifndef NEWSBOAT_REGEXOWNER_H_
#define NEWSBOAT_REGEXOWNER_H_

#include <memory>
#include <regex.h>
#include <string>
#include <sys/types.h>
#include <utility>
#include <vector>

namespace newsboat {

class Regex {
private:
	Regex(const regex_t& r);

public:
	~Regex();

	static std::unique_ptr<Regex> compile(std::string reg_expression,
		int regcomp_flags, std::string& error);
	std::vector<std::pair<int, int>> matches(std::string input, int max_matches,
			int flags) const;

private:
	regex_t regex;
};

} // namespace newsboat

#endif /* NEWSBOAT_REGEXOWNER_H_ */

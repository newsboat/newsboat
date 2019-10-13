#ifndef NEWSBOAT_MATCHEREXCEPTON_H_
#define NEWSBOAT_MATCHEREXCEPTON_H_

#include <stdexcept>
#include <string>

namespace newsboat {

class MatcherException : public std::exception {
public:
	enum class Type { ATTRIB_UNAVAIL, INVALID_REGEX };

	MatcherException(Type et,
		const std::string& info,
		const std::string& info2 = "")
		: type_(et)
		, addinfo(info)
		, addinfo2(info2)
	{}

	~MatcherException() throw() override {}
	const char* what() const throw() override;

private:
	Type type_;
	std::string addinfo;
	std::string addinfo2;
};

} // namespace newsboat

#endif /* NEWSBOAT_MATCHEREXCEPTON_H_ */

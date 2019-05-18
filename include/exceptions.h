#ifndef NEWSBOAT_EXCEPTIONS_H_
#define NEWSBOAT_EXCEPTIONS_H_

#include <sqlite3.h>
#include <stdexcept>
#include <string>

namespace newsboat {

class DbException : public std::exception {
public:
	explicit DbException(sqlite3* h)
		: msg(sqlite3_errmsg(h))
	{
	}
	~DbException() throw() override {}
	const char* what() const throw() override
	{
		return msg.c_str();
	}

private:
	std::string msg;
};

class MatcherException : public std::exception {
public:
	enum class Type { ATTRIB_UNAVAIL, INVALID_REGEX };

	MatcherException(Type et,
		const std::string& info,
		const std::string& info2 = "")
		: type_(et)
		, addinfo(info)
		, addinfo2(info2)
	{
	}

	~MatcherException() throw() override {}
	const char* what() const throw() override;

private:
	Type type_;
	std::string addinfo;
	std::string addinfo2;
};

} // namespace newsboat

#endif /* NEWSBOAT_EXCEPTIONS_H_ */

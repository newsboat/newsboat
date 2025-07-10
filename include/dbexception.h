#ifndef NEWSBOAT_DBEXCEPTION_H_
#define NEWSBOAT_DBEXCEPTION_H_

#include <sqlite3.h>
#include <string>

namespace Newsboat {

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

} // namespace Newsboat

#endif /* NEWSBOAT_DBEXCEPTION_H_ */


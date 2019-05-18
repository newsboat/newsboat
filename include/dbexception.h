#ifndef NEWSBOAT_DBEXCEPTION_H_
#define NEWSBOAT_DBEXCEPTION_H_

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

} // namespace newsboat

#endif /* NEWSBOAT_DBEXCEPTION_H_ */


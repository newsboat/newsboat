#ifndef NEWSBOAT_DBEXCEPTION_H_
#define NEWSBOAT_DBEXCEPTION_H_

#include <sqlite3.h>
#include <stdexcept>
#include <string>

#include "utf8string.h"

namespace newsboat {

class DbException : public std::exception {
public:
	explicit DbException(sqlite3* h)
	// SQLite guarantees that the result of `sqlite3_errmsg` is UTF-8:
	// https://sqlite.org/c3ref/errcode.html
		: msg(Utf8String::from_utf8(sqlite3_errmsg(h)))
	{
	}
	~DbException() throw() override {}
	const char* what() const throw() override
	{
		return msg.to_utf8().c_str();
	}

private:
	Utf8String msg;
};

} // namespace newsboat

#endif /* NEWSBOAT_DBEXCEPTION_H_ */


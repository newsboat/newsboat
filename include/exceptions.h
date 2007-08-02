#ifndef EXCEPTIONS_H_
#define EXCEPTIONS_H_

#include <stdexcept>
#include <string>

#include <sqlite3.h>

namespace newsbeuter {

class xmlexception : public std::exception {
	public:
		xmlexception(const std::string& errmsg) : msg(errmsg) { }
		virtual ~xmlexception() throw() { }
		virtual const char * what() const throw() { return msg.c_str(); }
	private:
		std::string msg;
};

class configexception : public std::exception {
	public:
		configexception(const std::string& errmsg) : msg(errmsg) { }
		virtual ~configexception() throw() { }
		virtual const char * what() const throw() { return msg.c_str(); }
	private:
		std::string msg;
};

class dbexception : public std::exception {
	public:
		dbexception(sqlite3 * h) : msg(sqlite3_errmsg(h)) { }
		virtual ~dbexception() throw() { }
		virtual const char * what() const throw() { return msg.c_str(); }
	private:
		std::string msg;
};

}

#endif /*EXCEPTIONS_H_*/

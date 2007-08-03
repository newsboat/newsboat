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

class matcherexception : public std::exception {
	public:
		enum errortype_t { ATTRIB_UNAVAIL };
		matcherexception(errortype_t et, const std::string& info) : type(et), addinfo(info) { }
		virtual ~matcherexception() throw() { }
		virtual const char * what() const throw();
	private:
		errortype_t type;
		std::string addinfo;
};

}

#endif /*EXCEPTIONS_H_*/

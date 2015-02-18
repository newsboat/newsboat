#ifndef EXCEPTIONS_H_
#define EXCEPTIONS_H_

#include <stdexcept>
#include <string>
#include <configparser.h>

#include <sqlite3.h>

namespace newsbeuter {

class xmlexception : public std::exception {
	public:
		xmlexception(const std::string& errmsg) : msg(errmsg) { }
		virtual ~xmlexception() throw() { }
		virtual const char * what() const throw() {
			return msg.c_str();
		}
	private:
		std::string msg;
};

class configexception : public std::exception {
	public:
		configexception(const std::string& errmsg) : msg(errmsg) { }
		virtual ~configexception() throw() { }
		virtual const char * what() const throw() {
			return msg.c_str();
		}
	private:
		std::string msg;
};

class confighandlerexception : public std::exception {
	public:
		confighandlerexception(const std::string& emsg) {
			msg = emsg;
		}
		confighandlerexception(action_handler_status e);
		virtual ~confighandlerexception() throw() { }
		virtual const char * what() const throw() {
			return msg.c_str();
		}
		int status() {
			return 0;
		}
	private:
		const char * get_errmsg(action_handler_status e);
		std::string msg;
};

class dbexception : public std::exception {
	public:
		dbexception(sqlite3 * h) : msg(sqlite3_errmsg(h)) { }
		virtual ~dbexception() throw() { }
		virtual const char * what() const throw() {
			return msg.c_str();
		}
	private:
		std::string msg;
};

class matcherexception : public std::exception {
	public:
		enum errortype_t { ATTRIB_UNAVAIL, INVALID_REGEX };
		matcherexception(errortype_t et, const std::string& info, const std::string& info2 = "") : type(et), addinfo(info), addinfo2(info2) { }
		virtual ~matcherexception() throw() { }
		virtual const char * what() const throw();
	private:
		errortype_t type;
		std::string addinfo;
		std::string addinfo2;
};

}

#endif /*EXCEPTIONS_H_*/

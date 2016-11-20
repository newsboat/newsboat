#ifndef EXCEPTIONS_H_
#define EXCEPTIONS_H_

#include <stdexcept>
#include <string>
#include <configparser.h>

#include <sqlite3.h>

namespace newsbeuter {

class xmlexception : public std::exception {
	public:
		explicit xmlexception(const std::string& errmsg) : msg(errmsg) { }
		virtual ~xmlexception() throw() { }
		virtual const char * what() const throw() {
			return msg.c_str();
		}
	private:
		std::string msg;
};

class configexception : public std::exception {
	public:
		explicit configexception(const std::string& errmsg) : msg(errmsg) { }
		virtual ~configexception() throw() { }
		virtual const char * what() const throw() {
			return msg.c_str();
		}
	private:
		std::string msg;
};

class confighandlerexception : public std::exception {
	public:
		explicit confighandlerexception(const std::string& emsg) : msg(emsg) { }
		explicit confighandlerexception(action_handler_status e);
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
		explicit dbexception(sqlite3 * h) : msg(sqlite3_errmsg(h)) { }
		virtual ~dbexception() throw() { }
		virtual const char * what() const throw() {
			return msg.c_str();
		}
	private:
		std::string msg;
};

class matcherexception : public std::exception {
	public:
		enum class type { ATTRIB_UNAVAIL, INVALID_REGEX };

		matcherexception(
				type et,
				const std::string& info,
				const std::string& info2 = "")
		: type_(et), addinfo(info), addinfo2(info2)
		{ }

		virtual ~matcherexception() throw() { }
		virtual const char * what() const throw();
	private:
		type type_;
		std::string addinfo;
		std::string addinfo2;
};

}

#endif /*EXCEPTIONS_H_*/

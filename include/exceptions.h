#ifndef EXCEPTIONS_H_
#define EXCEPTIONS_H_

#include <stdexcept>
#include <string>

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
	
}

#endif /*EXCEPTIONS_H_*/

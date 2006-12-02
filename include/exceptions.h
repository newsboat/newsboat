#ifndef EXCEPTIONS_H_
#define EXCEPTIONS_H_

#include <stdexcept>
#include <string>

namespace noos {

class xmlexception : public std::exception {
	public:
		xmlexception(const std::string& errmsg) : msg(errmsg) { }
		virtual ~xmlexception() throw() { }
		virtual const char * what() const throw() { return msg.c_str(); }
	private:
		std::string msg;
};
	
}

#endif /*EXCEPTIONS_H_*/

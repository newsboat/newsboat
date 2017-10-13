#ifndef NEWSBOAT_EXCEPTION_H_
#define NEWSBOAT_EXCEPTION_H_

#include <exception>

namespace newsboat {

class exception : public std::exception {
	public:
		explicit exception(unsigned int error_code = 0);
		~exception() throw();
		virtual const char* what() const throw();
	private:
		unsigned int ecode;
};

}

#endif /* NEWSBOAT_EXCEPTION_H_ */

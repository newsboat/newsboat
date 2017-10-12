#ifndef AK_EXCEPTION__H
#define AK_EXCEPTION__H

#include <exception>

namespace newsbeuter {

class exception : public std::exception {
	public:
		explicit exception(unsigned int error_code = 0);
		~exception() throw() override;
		const char* what() const throw() override;
	private:
		unsigned int ecode;
};

}


#endif

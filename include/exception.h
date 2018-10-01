#ifndef NEWSBOAT_EXCEPTION_H_
#define NEWSBOAT_EXCEPTION_H_

#include <exception>

namespace newsboat {

class Exception : public std::exception {
public:
	explicit Exception(unsigned int error_code = 0);
	~Exception() throw() override;
	const char* what() const throw() override;

private:
	unsigned int ecode;
};

} // namespace newsboat

#endif /* NEWSBOAT_EXCEPTION_H_ */

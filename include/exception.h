#ifndef NEWSBOAT_EXCEPTION_H_
#define NEWSBOAT_EXCEPTION_H_

#include <exception>

namespace newsboat {

class exception : public std::exception {
public:
	explicit exception(unsigned int error_code = 0);
	~exception() throw() override;
	const char* what() const throw() override;

private:
	unsigned int ecode;
};

} // namespace newsboat

#endif /* NEWSBOAT_EXCEPTION_H_ */

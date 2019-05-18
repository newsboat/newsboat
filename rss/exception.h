#ifndef NEWSBOAT_RSSPPEXCEPTION_H_
#define NEWSBOAT_RSSPPEXCEPTION_H_

#include <exception>
#include <string>

namespace rsspp {

class Exception : public std::exception {
public:
	explicit Exception(const std::string& errmsg = "");
	~Exception() throw() override;
	const char* what() const throw() override;

private:
	std::string emsg;
};

} // namespace rsspp

#endif /* NEWSBOAT_RSSPPEXCEPTION_H_ */

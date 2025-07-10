#ifndef NEWSBOAT_CONFIGHANDLEREXCEPTION_H_
#define NEWSBOAT_CONFIGHANDLEREXCEPTION_H_

#include <string>

namespace Newsboat {

enum class ActionHandlerStatus;

class ConfigHandlerException : public std::exception {
public:
	explicit ConfigHandlerException(const std::string& emsg)
		: msg(emsg)
	{
	}
	explicit ConfigHandlerException(ActionHandlerStatus e);
	~ConfigHandlerException() throw() override {}
	const char* what() const throw() override
	{
		return msg.c_str();
	}
	int status()
	{
		return 0;
	}

private:
	const char* get_errmsg(ActionHandlerStatus e);
	std::string msg;
};

} // namespace Newsboat

#endif /* NEWSBOAT_CONFIGHANDLEREXCEPTION_H_ */


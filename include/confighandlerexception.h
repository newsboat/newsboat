#ifndef NEWSBOAT_CONFIGHANDLEREXCEPTION_H_
#define NEWSBOAT_CONFIGHANDLEREXCEPTION_H_

#include <stdexcept>

#include "utf8string.h"

namespace newsboat {

enum class ActionHandlerStatus;

class ConfigHandlerException : public std::exception {
public:
	explicit ConfigHandlerException(const Utf8String& emsg)
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
	Utf8String get_errmsg(ActionHandlerStatus e);
	Utf8String msg;
};

} // namespace newsboat

#endif /* NEWSBOAT_CONFIGHANDLEREXCEPTION_H_ */


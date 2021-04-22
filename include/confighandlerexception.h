#ifndef NEWSBOAT_CONFIGHANDLEREXCEPTION_H_
#define NEWSBOAT_CONFIGHANDLEREXCEPTION_H_

#include <stdexcept>
#include <string>

#include "utf8string.h"

namespace newsboat {

enum class ActionHandlerStatus;

class ConfigHandlerException : public std::exception {
public:
	explicit ConfigHandlerException(const std::string& emsg)
		: msg(Utf8String::from_utf8(emsg))
	{
	}
	explicit ConfigHandlerException(ActionHandlerStatus e);
	~ConfigHandlerException() throw() override {}
	const char* what() const throw() override
	{
		return msg.to_utf8().c_str();
	}
	int status()
	{
		return 0;
	}

private:
	const char* get_errmsg(ActionHandlerStatus e);
	Utf8String msg;
};

} // namespace newsboat

#endif /* NEWSBOAT_CONFIGHANDLEREXCEPTION_H_ */


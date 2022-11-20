#ifndef NEWSBOAT_CONFIGEXCEPTION_H_
#define NEWSBOAT_CONFIGEXCEPTION_H_

#include <stdexcept>

#include "utf8string.h"

namespace newsboat {

class ConfigException : public std::exception {
public:
	explicit ConfigException(const Utf8String& errmsg)
		: msg(errmsg)
	{
	}
	~ConfigException() throw() override {}
	const char* what() const throw() override
	{
		return msg.c_str();
	}

private:
	Utf8String msg;
};

} // namespace newsboat

#endif /* NEWSBOAT_CONFIGEXCEPTION_H_ */


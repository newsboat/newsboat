#ifndef NEWSBOAT_CONFIGEXCEPTION_H_
#define NEWSBOAT_CONFIGEXCEPTION_H_

#include <stdexcept>
#include <string>

#include "utf8string.h"

namespace newsboat {

class ConfigException : public std::exception {
public:
	explicit ConfigException(const std::string& errmsg)
		: msg(Utf8String::from_utf8(errmsg))
	{
	}
	~ConfigException() throw() override {}
	const char* what() const throw() override
	{
		return msg.to_utf8().c_str();
	}

private:
	Utf8String msg;
};

} // namespace newsboat

#endif /* NEWSBOAT_CONFIGEXCEPTION_H_ */


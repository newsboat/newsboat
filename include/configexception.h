#ifndef NEWSBOAT_CONFIGEXCEPTION_H_
#define NEWSBOAT_CONFIGEXCEPTION_H_

#include <string>

namespace newsboat {

class ConfigException : public std::exception {
public:
	explicit ConfigException(const std::string& errmsg)
		: msg(errmsg)
	{
	}
	~ConfigException() throw() override {}
	const char* what() const throw() override
	{
		return msg.c_str();
	}

private:
	std::string msg;
};

} // namespace newsboat

#endif /* NEWSBOAT_CONFIGEXCEPTION_H_ */


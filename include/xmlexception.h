#ifndef NEWSBOAT_XMLEXCEPTION_H_
#define NEWSBOAT_XMLEXCEPTION_H_

#include <stdexcept>
#include <string>

namespace newsboat {

class XmlException : public std::exception {
public:
	explicit XmlException(const std::string& errmsg)
		: msg(errmsg)
	{}
	~XmlException() throw() override {}
	const char* what() const throw() override
	{
		return msg.c_str();
	}

private:
	std::string msg;
};

} // namespace newsboat

#endif /* NEWSBOAT_XMLEXCEPTION_H_ */

#include "matcherexception.h"

#include "config.h"
#include "strprintf.h"

namespace newsboat {

const char* MatcherException::what() const throw()
{
	static std::string errmsg;
	switch (type_) {
	case Type::ATTRIB_UNAVAIL:
		errmsg = strprintf::fmt(
				_("attribute `%s' is not available."), addinfo);
		break;
	case Type::INVALID_REGEX:
		errmsg = strprintf::fmt(
				_("regular expression '%s' is invalid: %s"),
				addinfo,
				addinfo2);
		break;
	}
	return errmsg.c_str();
}

} // namespace newsboat

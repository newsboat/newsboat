#include "matcherexception.h"

#include "config.h"
#include "strprintf.h"

namespace newsboat {

const char* MatcherException::what() const throw()
{
	static std::string errmsg;
	switch (type_) {
	case Type::AttributeUnavailable:
		errmsg = strprintf::fmt(
				_("attribute `%s' is not available."), addinfo);
		break;
	case Type::InvalidRegex:
		errmsg = strprintf::fmt(
				_("regular expression '%s' is invalid: %s"),
				addinfo,
				addinfo2);
		break;
	}
	return errmsg.c_str();
}

MatcherException MatcherException::from_rust_error(const
	matchererror::bridged::MatcherError& error)
{
	const auto error_ffi = matchererror::bridged::matcher_error_to_ffi(error);
	return MatcherException(
			error_ffi.err_type,
			std::string(error_ffi.info),
			std::string(error_ffi.info2));
}

} // namespace newsboat

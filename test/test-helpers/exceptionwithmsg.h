#ifndef NEWSBOAT_TEST_HELPERS_EXCEPTIONWITHMSG_H_
#define NEWSBOAT_TEST_HELPERS_EXCEPTIONWITHMSG_H_

#include <sstream>
#include <string>

#include "3rd-party/catch.hpp"

namespace TestHelpers {

/* \brief Matcher for an exception with specified message.
 *
 * This helper class can be used with assertion macros like
 * REQUIRE_THROWS_MATCHES:
 *
 *		REQUIRE_THROWS_MATCHES(
 *			call_that_throws(),
 *			exception_type,
 *			ExceptionWithMsg<exception_type>(expected_message));
 */
template<typename Exception>
class ExceptionWithMsg : public Catch::MatcherBase<Exception> {
	std::string expected_msg;

public:
	explicit ExceptionWithMsg(std::string&& msg)
		: expected_msg(std::move(msg))
	{
	}

	explicit ExceptionWithMsg(const std::string& msg)
		: expected_msg(msg)
	{
	}

	bool match(const Exception& e) const override
	{
		return expected_msg == e.what();
	}

	std::string describe() const override
	{
		std::ostringstream ss;
		ss << "should contain message \"" << expected_msg << "\"";
		return ss.str();
	}
};

} // namespace TestHelpers

#endif /* NEWSBOAT_TEST_HELPERS_EXCEPTIONWITHMSG_H_ */

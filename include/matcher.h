#ifndef NEWSBOAT_MATCHER_H_
#define NEWSBOAT_MATCHER_H_

#include "FilterParser.h"

namespace Newsboat {

class Matchable;

class Matcher {
public:
	Matcher();
	explicit Matcher(const std::string& expr);
	bool parse(const std::string& expr);
	bool matches(Matchable* item);
	std::string get_parse_error();
	std::string get_expression();

	/// Convert numerical prefix of the string to an `int`.
	///
	/// Return 0 if there is no numeric prefix. On underflow, return `int`'s
	/// minimum. On overflow, return `int`'s maximum.
	// This is made public so it can be tested. DON'T USE OUTSIDE OF MATCHER.
	static int string_to_num(const std::string& number);

private:
	bool matches_r(expression* e, Matchable* item);

	bool matchop_lt(expression* e, Matchable* item);
	bool matchop_gt(expression* e, Matchable* item);
	bool matchop_rxeq(expression* e, Matchable* item);
	bool matchop_cont(expression* e, Matchable* item);
	bool matchop_eq(expression* e, Matchable* item);
	bool matchop_between(expression* e, Matchable* item);

	FilterParser p;
	std::string errmsg;
	std::string exp;
};

} // namespace Newsboat

#endif /* NEWSBOAT_MATCHER_H_ */

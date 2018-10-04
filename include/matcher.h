#ifndef NEWSBOAT_MATCHER_H_
#define NEWSBOAT_MATCHER_H_

#include "FilterParser.h"

namespace newsboat {

class Matchable {
public:
	Matchable();
	virtual ~Matchable();
	virtual bool has_attribute(const std::string& attribname) = 0;
	virtual std::string get_attribute(const std::string& attribname) = 0;
};

class Matcher {
public:
	Matcher();
	explicit Matcher(const std::string& expr);
	bool parse(const std::string& expr);
	bool matches(Matchable* item);
	const std::string& get_parse_error();
	const std::string& get_expression();

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

} // namespace newsboat

#endif /* NEWSBOAT_MATCHER_H_ */

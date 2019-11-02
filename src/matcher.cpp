#include "matcher.h"

#include <cassert>
#include <cinttypes>
#include <ctime>
#include <regex.h>
#include <sstream>
#include <sys/time.h>
#include <vector>

#include "logger.h"
#include "matchable.h"
#include "matcherexception.h"
#include "scopemeasure.h"
#include "utils.h"

namespace newsboat {

extern "C" {
	void* rs_matcher_parse(const char *expr);
	bool rs_matcher_matches(void *matcher, const void *matchable);
	void rs_matcher_destroy(void *matcher);
}

Matchable::Matchable() {}
Matchable::~Matchable() {}

Matcher::Matcher() {}

Matcher::Matcher(const std::string& expr)
	: exp(expr)
{
	parse(expr);
}

Matcher::~Matcher() {
	rs_matcher_destroy(rs_matcher);
}

const std::string& Matcher::get_expression()
{
	return exp;
}

bool Matcher::parse(const std::string& expr)
{
	struct timeval tv1, tv2;
	gettimeofday(&tv1, nullptr);

	errmsg = "";

	void *m = rs_matcher_parse(expr.c_str());
	rs_matcher = m;

	gettimeofday(&tv2, nullptr);
	const uint64_t diff =
		(((tv2.tv_sec - tv1.tv_sec) * 1000000) + tv2.tv_usec) -
		tv1.tv_usec;
	LOG(Level::DEBUG,
		"Matcher::parse: parsing `%s' took %" PRIu64 " µs (success = %d)",
		expr,
		diff,
		m != nullptr);

	return m != nullptr;
}

bool Matcher::matches(Matchable* item)
{
	/*
	 * with this method, every class that is derived from Matchable can be
	 * matched against a filter expression that was previously passed to the
	 * class with the parse() method.
	 *
	 * This makes it easy to use the Matcher virtually everywhere, since C++
	 * allows multiple inheritance (i.e. deriving from Matchable can even be
	 * used in class hierarchies), and deriving from Matchable means that
	 * you only have to implement two methods has_attribute() and
	 * get_attribute().
	 *
	 * The whole matching code is speed-critical, as the matching happens on
	 * a lot of different occassions, and slow matching can be easily
	 * measured (and felt by the user) on slow computers with a lot of items
	 * to match.
	 */
	bool retval = false;
	if (item) {
		ScopeMeasure m1("Matcher::matches");
		retval = rs_matcher_matches(rs_matcher, item);
	}
	return retval;
}

bool Matcher::matchop_lt(expression* e, Matchable* item)
{
	if (!item->has_attribute(e->name))
		throw MatcherException(
			MatcherException::Type::ATTRIB_UNAVAIL, e->name);
	std::istringstream islit(e->literal);
	std::istringstream isatt(item->get_attribute(e->name));
	int ilit, iatt;
	islit >> ilit;
	isatt >> iatt;
	return iatt < ilit;
}

bool Matcher::matchop_between(expression* e, Matchable* item)
{
	if (!item->has_attribute(e->name))
		throw MatcherException(
			MatcherException::Type::ATTRIB_UNAVAIL, e->name);
	std::vector<std::string> lit = utils::tokenize(e->literal, ":");
	std::istringstream isatt(item->get_attribute(e->name));
	int att;
	isatt >> att;
	if (lit.size() < 2) {
		return false;
	}
	std::istringstream is1(lit[0]), is2(lit[1]);
	int i1, i2;
	is1 >> i1;
	is2 >> i2;
	if (i1 > i2) {
		int tmp = i1;
		i1 = i2;
		i2 = tmp;
	}
	return (att >= i1 && att <= i2);
}

bool Matcher::matchop_gt(expression* e, Matchable* item)
{
	if (!item->has_attribute(e->name))
		throw MatcherException(
			MatcherException::Type::ATTRIB_UNAVAIL, e->name);
	std::istringstream islit(e->literal);
	std::istringstream isatt(item->get_attribute(e->name));
	int ilit, iatt;
	islit >> ilit;
	isatt >> iatt;
	return iatt > ilit;
}

bool Matcher::matchop_rxeq(expression* e, Matchable* item)
{
	if (!item->has_attribute(e->name))
		throw MatcherException(
			MatcherException::Type::ATTRIB_UNAVAIL, e->name);
	if (!e->regex) {
		e->regex = new regex_t;
		int err;
		if ((err = regcomp(e->regex,
						e->literal.c_str(),
						REG_EXTENDED | REG_ICASE | REG_NOSUB)) != 0) {
			delete e->regex;
			e->regex = nullptr;
			char buf[1024];
			regerror(err, e->regex, buf, sizeof(buf));
			throw MatcherException(
				MatcherException::Type::INVALID_REGEX,
				e->literal,
				buf);
		}
	}
	if (regexec(e->regex,
			item->get_attribute(e->name).c_str(),
			0,
			nullptr,
			0) == 0) {
		return true;
	}
	return false;
}

bool Matcher::matchop_cont(expression* e, Matchable* item)
{
	if (!item->has_attribute(e->name))
		throw MatcherException(
			MatcherException::Type::ATTRIB_UNAVAIL, e->name);
	std::vector<std::string> elements =
		utils::tokenize(item->get_attribute(e->name), " ");
	std::string literal = e->literal;
	for (const auto& elem : elements) {
		if (literal == elem) {
			return true;
		}
	}
	return false;
}

bool Matcher::matchop_eq(expression* e, Matchable* item)
{
	if (!item->has_attribute(e->name)) {
		LOG(Level::WARN,
			"Matcher::matches_r: attribute %s not available",
			e->name);
		throw MatcherException(
			MatcherException::Type::ATTRIB_UNAVAIL, e->name);
	}
	return (item->get_attribute(e->name) == e->literal);
}

bool Matcher::matches_r(expression* e, Matchable* item)
{
	if (e) {
		switch (e->op) {
		/* the operator "and" and "or" simply connect two different
		 * subexpressions */
		case LOGOP_AND:
			// short-circuit evaluation in C -> short circuit evaluation in the filter language
			return matches_r(e->l, item) &&
				matches_r(e->r, item);

		case LOGOP_OR:
			return matches_r(e->l, item) ||
				matches_r(e->r, item); // same here

		/* while the other operator connect an attribute with a value */
		case MATCHOP_EQ:
			return matchop_eq(e, item);

		case MATCHOP_NE:
			return !matchop_eq(e, item);

		case MATCHOP_LT:
			return matchop_lt(e, item);

		case MATCHOP_BETWEEN:
			return matchop_between(e, item);

		case MATCHOP_GT:
			return matchop_gt(e, item);

		case MATCHOP_LE:
			return !matchop_gt(e, item);

		case MATCHOP_GE:
			return !matchop_lt(e, item);

		case MATCHOP_RXEQ:
			return matchop_rxeq(e, item);

		case MATCHOP_RXNE:
			return !matchop_rxeq(e, item);

		case MATCHOP_CONTAINS:
			return matchop_cont(e, item);

		case MATCHOP_CONTAINSNOT:
			return !matchop_cont(e, item);
		}
		return false;
	} else {
		return true; // shouldn't happen
	}
}

const std::string& Matcher::get_parse_error()
{
	return errmsg;
}

} // namespace newsboat

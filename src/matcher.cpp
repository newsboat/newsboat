#include "matcher.h"

#include <regex.h>
#include <sstream>
#include <vector>

#include "logger.h"
#include "matchable.h"
#include "matcherexception.h"
#include "scopemeasure.h"
#include "utils.h"

namespace Newsboat {

Matcher::Matcher() {}

Matcher::Matcher(const std::string& expr)
	: exp(expr)
{
	parse(expr);
}

std::string Matcher::get_expression()
{
	return exp;
}

bool Matcher::parse(const std::string& expr)
{
	ScopeMeasure measurer("Matcher::parse");

	errmsg = "";

	bool b = p.parse_string(expr);

	if (b) {
		exp = expr;
	} else {
		errmsg = utils::wstr2str(p.get_error());
	}

	LOG(Level::DEBUG,
		"Matcher::parse: parsing `%s' succeeded: %d",
		expr,
		b ? 1 : 0);

	return b;
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
	 * you only have to implement the method attribute_value().
	 *
	 * The whole matching code is speed-critical, as the matching happens on
	 * a lot of different occasions, and slow matching can be easily
	 * measured (and felt by the user) on slow computers with a lot of items
	 * to match.
	 */
	bool retval = false;
	if (item) {
		ScopeMeasure m1("Matcher::matches");
		retval = matches_r(p.get_root(), item);
	}
	return retval;
}

std::string get_attr_or_throw(Matchable* item, const std::string& attr_name)
{
	const auto attr = item->attribute_value(attr_name);

	if (!attr.has_value()) {
		LOG(Level::WARN,
			"Matcher::matches: attribute %s is not available",
			attr_name);
		throw MatcherException(MatcherException::Type::ATTRIB_UNAVAIL, attr_name);
	}

	return attr.value();
}

bool Matcher::matchop_lt(expression* e, Matchable* item)
{
	const int ilit = string_to_num(e->literal);

	const auto attr = get_attr_or_throw(item, e->name);
	const int iatt = string_to_num(attr);

	return iatt < ilit;
}

bool Matcher::matchop_between(expression* e, Matchable* item)
{
	const auto attr = get_attr_or_throw(item, e->name);
	const int att = string_to_num(attr);

	const std::vector<std::string> lit = utils::tokenize(e->literal, ":");
	if (lit.size() < 2) {
		return false;
	}

	int i1 = string_to_num(lit[0]);
	int i2 = string_to_num(lit[1]);
	if (i1 > i2) {
		std::swap(i1, i2);
	}

	return (att >= i1 && att <= i2);
}

bool Matcher::matchop_gt(expression* e, Matchable* item)
{
	const auto attr = get_attr_or_throw(item, e->name);
	const int iatt = string_to_num(attr);

	const int ilit = string_to_num(e->literal);

	return iatt > ilit;
}

bool Matcher::matchop_rxeq(expression* e, Matchable* item)
{
	const auto attr = get_attr_or_throw(item, e->name);

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
			attr.c_str(),
			0,
			nullptr,
			0) == 0) {
		return true;
	}
	return false;
}

bool Matcher::matchop_cont(expression* e, Matchable* item)
{
	const auto attr = get_attr_or_throw(item, e->name);

	const std::vector<std::string> elements = utils::tokenize(attr, " ");
	const std::string literal = e->literal;
	for (const auto& elem : elements) {
		if (literal == elem) {
			return true;
		}
	}
	return false;
}

bool Matcher::matchop_eq(expression* e, Matchable* item)
{
	const auto attr = get_attr_or_throw(item, e->name);

	return (attr == e->literal);
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

		/* while the other operators connect an attribute with a value */
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

std::string Matcher::get_parse_error()
{
	return errmsg;
}

int Matcher::string_to_num(const std::string& number)
{
	int result = 0;
	std::istringstream(number) >> result;
	return result;
}

} // namespace Newsboat

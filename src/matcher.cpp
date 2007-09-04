#include <matcher.h>
#include <logger.h>
#include <utils.h>
#include <exceptions.h>

#include <sys/time.h>
#include <ctime>
#include <cassert>
#include <sstream>
#include <vector>

namespace newsbeuter {

matchable::matchable() { }
matchable::~matchable() { }

matcher::matcher() { }

matcher::matcher(const std::string& expr) {
	parse(expr);
}

bool matcher::parse(const std::string& expr) {
	struct timeval tv1, tv2;
	gettimeofday(&tv1, NULL);

	bool b = p.parse_string(expr);

	gettimeofday(&tv2, NULL);
	unsigned long diff = (((tv2.tv_sec - tv1.tv_sec) * 1000000) + tv2.tv_usec) - tv1.tv_usec;
	GetLogger().log(LOG_DEBUG, "matcher::parse: parsing `%s' took %lu µs (success = %d)", expr.c_str(), diff, b ? 1 : 0);

	return b;
}

bool matcher::matches(matchable* item) {
	/*
	 * with this method, every class that is derived from matchable can be
	 * matched against a filter expression that was previously passed to the
	 * class with the parse() method.
	 *
	 * This makes it easy to use the matcher virtually everywhere, since C++
	 * allows multiple inheritance (i.e. deriving from matchable can even be
	 * used in class hierarchies), and deriving from matchable means that you
	 * only have to implement two methods has_attribute() and get_attribute().
	 *
	 * The whole matching code is speed-critical, as the matching happens on a
	 * lot of different occassions, and slow matching can be easily measured
	 * (and felt by the user) on slow computers with a lot of items to match.
	 */
	bool retval = false; if (item) { struct timeval tv1, tv2;
	gettimeofday(&tv1, NULL);

		retval = matches_r(p.get_root(), item);

		gettimeofday(&tv2, NULL);
		unsigned long diff = (((tv2.tv_sec - tv1.tv_sec) * 1000000) + tv2.tv_usec) - tv1.tv_usec;
		GetLogger().log(LOG_DEBUG, "matcher::matches matching took %lu µs", diff);
	}
	return retval;
}

bool matcher::matches_r(expression * e, matchable * item) {
	if (e) {
		bool retval;
		switch (e->op) {
			/* the operator "and" and "or" simply connect two different subexpressions */
			case LOGOP_AND:
				retval = matches_r(e->l, item);
				retval = retval && matches_r(e->r, item); // short-circuit evaulation in C -> short circuit evaluation in the filter language
				break;

			case LOGOP_OR:
				retval = matches_r(e->l, item);
				retval = retval || matches_r(e->r, item); // same here
				break;

			/* while the other operator connect an attribute with a value */
			case MATCHOP_EQ:
				if (item->has_attribute(e->name))
					retval = (item->get_attribute(e->name)==e->literal);
				else {
					GetLogger().log(LOG_WARN, "matcher::matches_r: attribute %s not available", e->name.c_str());
					throw matcherexception(matcherexception::ATTRIB_UNAVAIL, e->name);
				}
				break;

			case MATCHOP_NE:
				if (item->has_attribute(e->name))
					retval = (item->get_attribute(e->name)!=e->literal);
				else {
					GetLogger().log(LOG_WARN, "matcher::matches_r: attribute %s not available", e->name.c_str());
					throw matcherexception(matcherexception::ATTRIB_UNAVAIL, e->name);
				}
				break;

			case MATCHOP_LT: {
					if (!item->has_attribute(e->name))
						throw matcherexception(matcherexception::ATTRIB_UNAVAIL, e->name);
					std::istringstream islit(e->literal);
					std::istringstream isatt(item->get_attribute(e->name));
					int ilit, iatt;
					islit >> ilit;
					isatt >> iatt;
					return iatt < ilit;
				}
				break;

			case MATCHOP_GT: {
					if (!item->has_attribute(e->name))
						throw matcherexception(matcherexception::ATTRIB_UNAVAIL, e->name);
					std::istringstream islit(e->literal);
					std::istringstream isatt(item->get_attribute(e->name));
					int ilit, iatt;
					islit >> ilit;
					isatt >> iatt;
					return iatt > ilit;
				}
				break;

			case MATCHOP_LE: {
					if (!item->has_attribute(e->name))
						throw matcherexception(matcherexception::ATTRIB_UNAVAIL, e->name);
					std::istringstream islit(e->literal);
					std::istringstream isatt(item->get_attribute(e->name));
					int ilit, iatt;
					islit >> ilit;
					isatt >> iatt;
					return iatt <= ilit;
				}
				break;

			case MATCHOP_GE: {
					if (!item->has_attribute(e->name))
						throw matcherexception(matcherexception::ATTRIB_UNAVAIL, e->name);
					std::istringstream islit(e->literal);
					std::istringstream isatt(item->get_attribute(e->name));
					int ilit, iatt;
					islit >> ilit;
					isatt >> iatt;
					return iatt >= ilit;
				}
				break;

			case MATCHOP_RXEQ: {
					if (!item->has_attribute(e->name))
						throw matcherexception(matcherexception::ATTRIB_UNAVAIL, e->name);
					if (!e->regex) {
						e->regex = new regex_t;
						regcomp(e->regex, e->literal.c_str(), REG_EXTENDED | REG_ICASE | REG_NOSUB); // TODO: see below
					}
					if (regexec(e->regex, item->get_attribute(e->name).c_str(), 0, NULL, 0)==0)
						retval = true;
					else
						retval = false;
				}
				break;

			case MATCHOP_RXNE: {
					if (!item->has_attribute(e->name))
						throw matcherexception(matcherexception::ATTRIB_UNAVAIL, e->name);
					if (!e->regex) {
						e->regex = new regex_t;
						regcomp(e->regex, e->literal.c_str(), REG_EXTENDED | REG_ICASE | REG_NOSUB); // TODO: throw error when compilation fails
					}
					GetLogger().log(LOG_DEBUG, "matchop_rxne: %s !~ %s ?", item->get_attribute(e->name).c_str(), e->literal.c_str());
					if (regexec(e->regex, item->get_attribute(e->name).c_str(), 0, NULL, 0)==0) {
						GetLogger().log(LOG_DEBUG, "matchop_rxne: yes");
						retval = false;
					} else {
						GetLogger().log(LOG_DEBUG, "matchop_rxne: no");
						retval = true;
					}
				}
				break;

			case MATCHOP_CONTAINS: {
					if (!item->has_attribute(e->name))
						throw matcherexception(matcherexception::ATTRIB_UNAVAIL, e->name);
					std::vector<std::string> elements = utils::tokenize(item->get_attribute(e->name), " ");
					std::string literal = e->literal;
					retval = false;
					for (std::vector<std::string>::iterator it=elements.begin();it!=elements.end();++it) {
						if (literal == *it) {
							retval = true;
							break;
						}
					}
				}
				break;

			case MATCHOP_CONTAINSNOT: {
					if (!item->has_attribute(e->name))
						throw matcherexception(matcherexception::ATTRIB_UNAVAIL, e->name);
					std::vector<std::string> elements = utils::tokenize(item->get_attribute(e->name), " ");
					std::string literal = e->literal;
					retval = true;
					for (std::vector<std::string>::iterator it=elements.begin();it!=elements.end();++it) {
						if (literal == *it) {
							retval = false;
							break;
						}
					}
				}
				break;

			default:
				GetLogger().log(LOG_ERROR, "matcher::matches_r: invalid operator %d", e->op);
				assert(false); // that's an error condition
		}
		return retval;
	} else {
		return true; // shouldn't happen
	}
}

}

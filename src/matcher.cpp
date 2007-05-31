#include <matcher.h>
#include <logger.h>
#include <cassert>

namespace newsbeuter {

matchable::matchable() { }
matchable::~matchable() { }

matcher::matcher() { }

bool matcher::parse(const std::string& expr) {
	return p.parse_string(expr);
}

bool matcher::matches(matchable* item) {
	if (item) {
		return matches_r(p.get_root(), item);
	} else {
		return false; // shouldn't happen
	}
}

bool matcher::matches_r(expression * e, matchable * item) {
	if (e) {
		bool retval;
		switch (e->op) {
			case LOGOP_AND:
				retval = matches_r(e->l, item);
				retval = retval && matches_r(e->r, item); // short-circuit evaulation in C -> short circuit evaluation in the filter language
				break;
			case LOGOP_OR:
				retval = matches_r(e->l, item);
				retval = retval || matches_r(e->r, item); // same here
				break;
			case MATCHOP_EQ:
				if (item->has_attribute(e->name))
					retval = (item->get_attribute(e->name)==e->literal);
				else
					retval = false;
				break;
			case MATCHOP_NE:
				if (item->has_attribute(e->name))
					retval = (item->get_attribute(e->name)!=e->literal);
				else
					retval = false;
				break;
			case MATCHOP_RXEQ:
				retval = false; // TODO: implement
				break;
			case MATCHOP_RXNE:
				retval = false;
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

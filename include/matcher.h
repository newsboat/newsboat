#ifndef NEWSBEUTER_MATCHER__H
#define NEWSBEUTER_MATCHER__H

#include <FilterParser.h>

namespace newsbeuter {

	class matchable {
		public:
			matchable();
			virtual ~matchable();
			virtual bool has_attribute(const std::string& attribname) = 0;
			virtual std::string get_attribute(const std::string& attribname) = 0;
	};

	class matcher {
		public:
			matcher();
			matcher(const std::string& expr);
			bool parse(const std::string& expr);
			bool matches(matchable* item);
			const std::string& get_parse_error();
			const std::string& get_expression();
		private:
			bool matches_r(expression * e, matchable * item);

			bool matchop_lt(expression * e, matchable * item);
			bool matchop_gt(expression * e, matchable * item);
			bool matchop_rxeq(expression * e, matchable * item);
			bool matchop_cont(expression * e, matchable * item);
			bool matchop_eq(expression * e, matchable * item);
			bool matchop_between(expression * e, matchable * item);

			FilterParser p;
			std::string errmsg;
			std::string exp;
	};

}

#endif

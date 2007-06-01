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
		private:
			bool matches_r(expression * e, matchable * item);

			FilterParser p;
			bool success;
	};

}

#endif

#ifndef NEWSBEUTER_HISTORY__H
#define NEWSBEUTER_HISTORY__H

#include <list>
#include <string>

namespace newsbeuter {

	class history {
		public:
			history();
			~history();
			void add_line(const std::string& line);
			std::string prev();
			std::string next();
		private:
			std::list<std::string> lines;
			std::list<std::string>::const_iterator it;
	};

}

#endif

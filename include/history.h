#ifndef NEWSBEUTER_HISTORY__H
#define NEWSBEUTER_HISTORY__H

#include <vector>
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
			std::vector<std::string> lines;
			unsigned int idx;
	};

}

#endif

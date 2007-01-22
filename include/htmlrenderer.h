#ifndef NEWSBEUTER_HTMLRENDERER__H
#define NEWSBEUTER_HTMLRENDERER__H

#include <vector>
#include <string>
#include <istream>

namespace newsbeuter {

	class htmlrenderer {
		public:
			htmlrenderer(unsigned int width = 80);
			void render(const std::string&, std::vector<std::string>& lines,  std::vector<std::string>& links );
			void render(std::istream &, std::vector<std::string>& lines, std::vector<std::string>& links );
		private:
			unsigned int w;
			void prepare_newline(std::string& line, int indent_level);
	};

}

#endif

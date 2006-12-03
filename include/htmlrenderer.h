#ifndef NOOS_HTMLRENDERER__H
#define NOOS_HTMLRENDERER__H

#include <vector>
#include <string>

namespace noos {

	class htmlrenderer {
		public:
			htmlrenderer(unsigned int width = 80);
			std::vector<std::string> render(const std::string& source);
		private:
			unsigned int w;
			void prepare_newline(std::string& line, int indent_level);
	};

}

#endif

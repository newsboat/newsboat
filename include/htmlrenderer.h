#ifndef NOOS_HTMLRENDERER__H
#define NOOS_HTMLRENDERER__H

#include <vector>
#include <string>
#include <istream>

namespace noos {

	class htmlrenderer {
		public:
			htmlrenderer(unsigned int width = 80);
			void render(const std::string&, std::vector<std::string>& );
			void render(std::istream &, std::vector<std::string>& );
		private:
			unsigned int w;
			void prepare_newline(std::string& line, int indent_level);
	};

}

#endif

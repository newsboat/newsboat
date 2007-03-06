#ifndef NEWSBEUTER_HTMLRENDERER__H
#define NEWSBEUTER_HTMLRENDERER__H

#include <vector>
#include <string>
#include <istream>

namespace newsbeuter {

	class htmlrenderer {
		public:
			htmlrenderer(unsigned int width = 80);
			void render(const std::string&, std::vector<std::string>& lines,  std::vector<std::string>& links, const std::string& url);
			void render(std::istream &, std::vector<std::string>& lines, std::vector<std::string>& links, const std::string& url);
		private:
			unsigned int w;
			void prepare_newline(std::string& line, int indent_level);
			void add_link(std::vector<std::string>& links, const std::string& link);
			std::string absolute_url(const std::string& url, const std::string& link);
	};

}

#endif

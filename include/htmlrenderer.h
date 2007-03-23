#ifndef NEWSBEUTER_HTMLRENDERER__H
#define NEWSBEUTER_HTMLRENDERER__H

#include <vector>
#include <string>
#include <istream>

namespace newsbeuter {

	enum link_type { LINK_HREF, LINK_IMG, LINK_EMBED };

	typedef std::pair<std::string,link_type> linkpair;

	class htmlrenderer {
		public:
			htmlrenderer(unsigned int width = 80);
			void render(const std::string&, std::vector<std::string>& lines,  std::vector<linkpair>& links, const std::string& url);
			void render(std::istream &, std::vector<std::string>& lines, std::vector<linkpair>& links, const std::string& url);
		private:
			unsigned int w;
			void prepare_newline(std::string& line, int indent_level);
			unsigned int add_link(std::vector<linkpair>& links, const std::string& link, link_type type);
			std::string absolute_url(const std::string& url, const std::string& link);
			std::string type2str(link_type type);
	};

}

#endif

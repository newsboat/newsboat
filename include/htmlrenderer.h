#ifndef NEWSBEUTER_HTMLRENDERER__H
#define NEWSBEUTER_HTMLRENDERER__H

#include <vector>
#include <string>
#include <istream>
#include <map>

namespace newsbeuter {

	enum link_type { LINK_HREF, LINK_IMG, LINK_EMBED };
	enum htmltag { TAG_A = 1, TAG_EMBED, TAG_BR, TAG_PRE, TAG_ITUNESHACK, TAG_IMG, TAG_BLOCKQUOTE, TAG_P, TAG_OL, TAG_UL, TAG_LI, TAG_DT, TAG_DD, TAG_DL, TAG_SUP, TAG_SUB, TAG_HR, TAG_STRONG, TAG_UNDERLINE };

	typedef std::pair<std::string,link_type> linkpair;

	class htmlrenderer {
		public:
			htmlrenderer(unsigned int width = 80, bool raw = false);
			void render(const std::string&, std::vector<std::string>& lines,  std::vector<linkpair>& links, const std::string& url);
			void render(std::istream &, std::vector<std::string>& lines, std::vector<linkpair>& links, const std::string& url);
		private:
			unsigned int w;
			void prepare_newline(std::string& line, int indent_level);
			bool line_is_nonempty(const std::string& line);
			unsigned int add_link(std::vector<linkpair>& links, const std::string& link, link_type type);
			std::string quote_for_stfl(std::string str);
			std::string absolute_url(const std::string& url, const std::string& link);
			std::string type2str(link_type type);
			std::map<std::string, htmltag> tags;
			bool raw_;
	};

}

#endif

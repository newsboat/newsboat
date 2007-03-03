#ifndef PLANET_HTMLTMPL__H
#define PLANET_HTMLTMPL__H

#include <vector>
#include <string>
#include <map>
#include <iostream>
#include <rss.h>

namespace planet {

	enum nodetype { NT_ROOT, NT_TEXT, NT_LOOP, NT_IF, NT_VAR };

	typedef std::map<std::string, std::string> varmap;

	class htmltmpl {
		public:
			htmltmpl();
			~htmltmpl();
			static htmltmpl parse(const std::string& str);
			static htmltmpl parse_file(const std::string& file);
			std::string to_html(const std::vector<newsbeuter::rss_feed>& feeds, 
								const std::vector<newsbeuter::rss_item>& items, 
								varmap variables);
		private:
			static htmltmpl parse_stream(std::istream& is, const std::string& endtag = "");
			nodetype type;
			std::string text;
			std::vector<htmltmpl> children;
	};


}

#endif

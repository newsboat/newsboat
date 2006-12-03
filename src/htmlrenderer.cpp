#include <htmlrenderer.h>
#include <xmlpullparser.h>
#include <sstream>

using namespace noos;

htmlrenderer::htmlrenderer(unsigned int width) : w(width) { }

std::vector<std::string> htmlrenderer::render(const std::string& source) {
	std::vector<std::string> lines;
	std::vector<std::string> links;
	unsigned int link_count = 0;
	std::string curline;
	int indent_level = 0;
	
	std::istringstream input(source);
	xmlpullparser xpp;
	xpp.setInput(input);
	
	for (xmlpullparser::event e = xpp.next(); e != xmlpullparser::END_DOCUMENT; e = xpp.next()) {	
		switch (e) {
			case xmlpullparser::START_TAG:
				if (xpp.getText() == "a") {
					std::string link = xpp.getAttributeValue("href");
					if (link.length() > 0) {
						links.push_back(link);
						std::ostringstream ref;
						ref << "[" << link_count << "]";
						link_count++;
						curline.append(ref.str());
					}
				} else if (xpp.getText() == "br") {
					if (curline.length() > 0)
						lines.push_back(curline);
					prepare_newline(curline, indent_level);	
				} else if (xpp.getText() == "img") {
					std::string imgurl = xpp.getAttributeValue("src");
					if (imgurl.length() > 0) {
						links.push_back(imgurl);
						std::ostringstream ref;
						ref << "[" << link_count << "]";
						link_count++;
						curline.append(ref.str());
					}					
				} else if (xpp.getText() == "blockquote") {
					++indent_level;
					if (curline.length() > 0)
						lines.push_back(curline);
					lines.push_back(std::string(""));
					prepare_newline(curline, indent_level);	
				} else if (xpp.getText() == "p") {
					if (curline.length() > 0)
						lines.push_back(curline);
					lines.push_back(std::string(""));
					prepare_newline(curline, indent_level);	
				}
				break;
			case xmlpullparser::END_TAG:
				if (xpp.getText() == "blockquote") {
					--indent_level;
					if (indent_level < 0)
						indent_level = 0;
					if (curline.length() > 0)
						lines.push_back(curline);
					lines.push_back(std::string(""));
					prepare_newline(curline, indent_level);
				}
				break;
			case xmlpullparser::TEXT:
				{
					std::vector<std::string> words = xmlpullparser::tokenize(xpp.getText());
					unsigned int i=0;
					for (std::vector<std::string>::iterator it=words.begin();it!=words.end();++it,++i) {
						if ((curline.length() + it->length()) >= w) {
							lines.push_back(curline);
							prepare_newline(curline, indent_level);
						}
						curline.append(*it);
						if (i < words.size()-1)
							curline.append(" ");
					}
				}
				break;
			default:
				/* do nothing */
				break;
		}
	}
	lines.push_back(curline);
	
	if (links.size() > 0) {
		lines.push_back(std::string(""));
		lines.push_back(std::string("Links: "));
		for (unsigned int i=0;i<links.size();++i) {
			std::ostringstream line;
			line << "[" << i << "]: " << links[i];
			lines.push_back(line.str());
		}
	}

	return lines;
}

void htmlrenderer::prepare_newline(std::string& line, int indent_level) {
	line = "";
	for (int i=0;i<indent_level;++i) {
		line.append("  ");	
	}
}

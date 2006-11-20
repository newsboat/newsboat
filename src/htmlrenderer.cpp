#include <htmlrenderer.h>
#include <iostream>

using namespace noos;

htmlrenderer::htmlrenderer(unsigned int width) : w(width) { }

std::vector<std::string> htmlrenderer::render(const std::string& source) {
	std::vector<std::string> lines;
	std::string curline;
	std::string cleaned_source;

	const char * begin = source.c_str();
	bool addchar = true;

	// clean up source:
	while (*begin) {
		if (addchar) {
			if (*begin != '<') {
				char str[2];
				str[0] = *begin;
				str[1] = '\0';
				if (str[0] == '\n' || str[0] == '\r') {
					str[0] = ' ';
				}
				cleaned_source.append(str);
			} else {
				addchar = false;
			}
		} else {
			if (*begin == '>') {
				addchar = true;
				cleaned_source.append(" ");
			}
		}
		++begin;
	}

	// std::cerr << "cleaned_source: `" << cleaned_source << "'" << std::endl;

	begin = cleaned_source.c_str();
	while (strlen(begin) >= w) {
		const char * end = begin + w;
		while (end > begin && *end != ' ') 
			--end;
		if (begin == end) {
			char x[w+1];
			strncpy(x,begin,w);
			x[w] = '\0';
			curline = x;
		} else {
			char x[end-begin+1];
			strncpy(x,begin,end-begin);
			x[end-begin] = '\0';
			curline = x;
		}
		lines.push_back(curline);
		begin += curline.length() + 1;
	}
	lines.push_back(std::string(begin));
	return lines;
}

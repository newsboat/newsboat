#include <htmlrenderer.h>

using namespace noos;

htmlrenderer::htmlrenderer(unsigned int width) : w(width) { }

std::vector<std::string> htmlrenderer::render(const std::string& source) {
	std::vector<std::string> lines;
	std::string curline, cleaned_source;

	const char * begin = source.c_str();
	bool addchar = true;

	// clean up source:
	while (*begin) {
		if (addchar) {
			if (*begin != '<') {
				char str[2];
				str[0] = *begin;
				str[1] = '\0';
				cleaned_source.append(str);
			} else {
				addchar = false;
			}
		} else {
			if (*begin == '>') {
				addchar = true;
			}
		}
		++begin;
	}

	begin = cleaned_source.c_str();
	while (strlen(begin) >= w) {
		const char * end = begin + w;
		while (end > begin && !strchr(" \t\r\n",*end)) ++end;
		if (begin == end) {
			char x[w+1];
			strncpy(x,begin,w);
			x[w] = '\0';
			curline = x;
		} else {
			char x[end-begin+1];
			strncpy(x,begin,end-begin);
			x[w] = '\0';
			curline = x;
		}
		lines.push_back(curline);
		begin += w;
	}
	lines.push_back(begin);
	return lines;
}

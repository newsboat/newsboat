#include <formatstring.h>
#include <utils.h>

#include <sstream>
#include <vector>


namespace newsbeuter {

void fmtstr_formatter::register_fmt(char f, const std::string& value) {
	fmts[f] = value;
}

std::string fmtstr_formatter::do_format(const std::string& fmt, unsigned int width) {
	std::string result;
	unsigned int i;
	unsigned int fmtlen = fmt.length();
	for (i=0;i<fmtlen;++i) {
		if (fmt[i] == '%') {
			if (i<(fmtlen-1)) {
				if (fmt[i+1] == '-' || isdigit(fmt[i+1])) {
					std::string number;
					char c;
					while ((fmt[i+1] == '-' || isdigit(fmt[i+1])) && i<(fmtlen-1)) {
						number.append(1,fmt[i+1]);
						++i;
					}
					if (i<(fmtlen-1)) {
						c = fmt[i+1];
						++i;
						std::istringstream is(number);
						int align;
						is >> align;
						if (abs(align) > fmts[c].length()) {
							char buf[256];
							snprintf(buf,sizeof(buf),"%*s", align, fmts[c].c_str());
							result.append(buf);
						} else {
							result.append(fmts[c].substr(0,abs(align)));
						}
					}
				} else if (fmt[i+1] == '%') {
					result.append(1, '%');
					++i;
				} else if (fmt[i+1] == '>') {
					if (fmt[i+2]) {
						if (width == 0) {
							result.append(1, fmt[i+2]);
							i += 2;
						} else {
							std::string rightside = do_format(&fmt[i+3], 0);
							int diff = width - result.length() - rightside.length();
							if (diff > 0) {
								result.append(diff, fmt[i+2]);
							}
							result.append(rightside);
							i = fmtlen;
						}
					}
				} else if (fmt[i+1] == '?') {
					unsigned int j = i+2;
					while (fmt[j] && fmt[j] != '?')
						j++;
					if (fmt[j]) {
						std::string cond = fmt.substr(i+2, j - i - 1);
						unsigned int k = j + 1;
						while (fmt[k] && fmt[k] != '?')
							k++;
						if (fmt[k]) {
							std::string values = fmt.substr(j+1, k - j - 1);
							std::vector<std::string> pair = utils::tokenize(values,"&");
							while (pair.size() < 2)
								pair.push_back("");

							if (fmts[cond[0]].length() > 0) {
								result.append(do_format(pair[0], width));
							} else {
								result.append(do_format(pair[1], width));
							}

							i = k;
						} else {
							i = k - 1;
						}
					} else {
						i = j - 1;
					}
				} else {
					result.append(fmts[fmt[i+1]]);
					++i;
				}
			}
		} else {
			result.append(1, fmt[i]);
		}
	}
	return result;
}


}

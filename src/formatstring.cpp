#include <formatstring.h>

#include <sstream>


namespace newsbeuter {

void fmtstr_formatter::register_fmt(char f, const std::string& value) {
	fmts[f] = value;
}

std::string fmtstr_formatter::do_format(const std::string& fmt) {
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

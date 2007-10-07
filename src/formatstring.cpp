#include <formatstring.h>
#include <utils.h>
#include <logger.h>

#include <sstream>
#include <vector>


namespace newsbeuter {

void fmtstr_formatter::register_fmt(char f, const std::string& value) {
	GetLogger().log(LOG_DEBUG, "fmtstr_formatter::register_fmt: char = %c value = %s", f, value.c_str());
	fmts[f] = utils::str2wstr(value);
	GetLogger().log(LOG_DEBUG, "fmtstr_formatter::register_fmt: char = %c done", f);
}

std::string fmtstr_formatter::do_format(const std::string& fmt, unsigned int width) {
	std::wstring wfmt(utils::str2wstr(fmt));
	std::string result = utils::wstr2str(do_wformat(wfmt, width));
	GetLogger().log(LOG_DEBUG, "fmtstr_formatter::do_format: after conversion: `%s'", result.c_str());
	return result;
}

std::wstring fmtstr_formatter::do_wformat(const std::wstring& wfmt, unsigned int width) {
	GetLogger().log(LOG_DEBUG, "fmtstr_formatter::do_format: fmt = `%ls' width = %u", wfmt.c_str(), width);
	std::wstring result;
	unsigned int i;
	unsigned int fmtlen = wfmt.length();
	GetLogger().log(LOG_DEBUG, "fmtstr_formatter::do_format: fmtlen = %u", fmtlen);
	for (i=0;i<fmtlen;++i) {
		if (wfmt[i] == L'%') {
			if (i<(fmtlen-1)) {
				if (wfmt[i+1] == L'-' || iswdigit(wfmt[i+1])) {
					std::string number;
					wchar_t c;
					while ((wfmt[i+1] == L'-' || iswdigit(wfmt[i+1])) && i<(fmtlen-1)) {
						number.append(1,wfmt[i+1]);
						++i;
					}
					GetLogger().log(LOG_DEBUG, "fmtstr_formatter::do_format: number = %s", number.c_str());
					if (i<(fmtlen-1)) {
						c = wfmt[i+1];
						++i;
						std::istringstream is(number);
						int align;
						is >> align;
						if (abs(align) > fmts[c].length()) {
							wchar_t buf[256];
							swprintf(buf,sizeof(buf)/sizeof(*buf),L"%*ls", align, fmts[c].c_str());
							GetLogger().log(LOG_DEBUG, "fmtstr_formatter::do_format: swprintf result = %ls", buf);
							result.append(buf);
						} else {
							result.append(fmts[c].substr(0,abs(align)));
						}
					}
				} else if (wfmt[i+1] == L'%') {
					result.append(1, L'%');
					GetLogger().log(LOG_DEBUG, "fmtstr_formatter::do_format: appending %");
					++i;
				} else if (wfmt[i+1] == L'>') {
					if (wfmt[i+2]) {
						if (width == 0) {
							result.append(1, wfmt[i+2]);
							i += 2;
						} else {
							std::wstring rightside = do_wformat(&wfmt[i+3], 0);
							GetLogger().log(LOG_DEBUG, "fmtstr_formatter::do_format: aligning, right side = %ls", rightside.c_str());
							int diff = width - wcswidth(result.c_str(),result.length()) - wcswidth(rightside.c_str(), rightside.length());
							GetLogger().log(LOG_DEBUG, "fmtstr_formatter::do_format: diff = %d char = %lc", diff, wfmt[i+2]);
							if (diff > 0) {
								result.append(diff, wfmt[i+2]);
							}
							result.append(rightside);
							i = fmtlen;
						}
					}
				} else if (wfmt[i+1] == L'?') {
					unsigned int j = i+2;
					while (wfmt[j] && wfmt[j] != L'?')
						j++;
					if (wfmt[j]) {
						std::wstring cond = wfmt.substr(i+2, j - i - 1);
						unsigned int k = j + 1;
						while (wfmt[k] && wfmt[k] != L'?')
							k++;
						if (wfmt[k]) {
							std::wstring values = wfmt.substr(j+1, k - j - 1);
							std::vector<std::string> pair = utils::tokenize(utils::wstr2str(values),"&");
							while (pair.size() < 2)
								pair.push_back("");

							if (fmts[cond[0]].length() > 0) {
								result.append(do_wformat(utils::str2wstr(pair[0]), width));
							} else {
								result.append(do_wformat(utils::str2wstr(pair[1]), width));
							}

							i = k;
						} else {
							i = k - 1;
						}
					} else {
						i = j - 1;
					}
				} else {
					result.append(fmts[wfmt[i+1]]);
					++i;
				}
			}
		} else {
			result.append(1, wfmt[i]);
		}
	}
	return result;
}


}

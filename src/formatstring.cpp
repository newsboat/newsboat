#include <formatstring.h>
#include <utils.h>
#include <logger.h>

#include <sstream>
#include <vector>
#include <cstdlib>


namespace newsbeuter {

void fmtstr_formatter::register_fmt(char f, const std::string& value) {
	fmts[f] = utils::str2wstr(value);
}

std::string fmtstr_formatter::do_format(const std::string& fmt, unsigned int width) {
	std::string result;
	if (fmt.length() > 0) {
		std::wstring wfmt(utils::str2wstr(fmt));
		std::wstring w = do_wformat(wfmt, width);
		result = utils::wstr2str(w);
	}
	return result;
}

std::wstring fmtstr_formatter::do_wformat(const std::wstring& wfmt, unsigned int width) {
	std::wstring result;
	unsigned int i;
	unsigned int fmtlen = wfmt.length();
	for (i=0; i<fmtlen; ++i) {
		if (wfmt[i] == L'%') {
			if (i<(fmtlen-1)) {
				if (wfmt[i+1] == L'-' || iswdigit(wfmt[i+1])) {
					std::wstring number;
					while ((wfmt[i+1] == L'-' || iswdigit(wfmt[i+1])) && i<(fmtlen-1)) {
						number.append(1,wfmt[i+1]);
						++i;
					}
					if (i<(fmtlen-1)) {
						wchar_t c = wfmt[i+1];
						++i;
						std::wistringstream is(number);
						int align;
						is >> align;
						if (static_cast<unsigned int>(abs(align)) > fmts[c].length()) {
							wchar_t buf[256];
							swprintf(buf,sizeof(buf)/sizeof(*buf),L"%*ls", align, fmts[c].c_str());
							result.append(buf);
						} else {
							result.append(fmts[c].substr(0,abs(align)));
						}
					}
				} else if (wfmt[i+1] == L'%') {
					result.append(1, L'%');
					++i;
				} else if (wfmt[i+1] == L'>') {
					if (wfmt[i+2]) {
						if (width == 0) {
							result.append(1, wfmt[i+2]);
							i += 2;
						} else {
							std::wstring rightside = do_wformat(&wfmt[i+3], 0);
							int diff = width - wcswidth(result.c_str(),result.length()) - wcswidth(rightside.c_str(), rightside.length());
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
						std::wstring cond = wfmt.substr(i+2, j - i - 2);
						unsigned int k = j + 1;
						while (wfmt[k] && wfmt[k] != L'?')
							k++;
						if (wfmt[k]) {
							std::wstring values = wfmt.substr(j+1, k - j - 1);
							std::vector<std::wstring> pair = utils::wtokenize(values,L"&");
							while (pair.size() < 2)
								pair.push_back(L"");

							std::wstring subresult;
							if (fmts[cond[0]].length() > 0) {
								if (pair[0].length() > 0)
									subresult = do_wformat(pair[0], width);
							} else {
								if (pair[1].length() > 0)
									subresult = do_wformat(pair[1], width);
							}
							result.append(subresult);
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

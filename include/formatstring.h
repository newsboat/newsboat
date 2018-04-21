#ifndef NEWSBOAT_FORMATSTRING_H_
#define NEWSBOAT_FORMATSTRING_H_

#include <map>
#include <string>

namespace newsboat {

class fmtstr_formatter {
public:
	fmtstr_formatter() {}
	void register_fmt(char f, const std::string& value);
	std::string do_format(const std::string& fmt, unsigned int width = 0);
	std::wstring
	do_wformat(const std::wstring& fmt, unsigned int width = 0);

private:
	std::map<char, std::wstring> fmts;
};

} // namespace newsboat

#endif /* NEWSBOAT_FORMATSTRING_H_ */

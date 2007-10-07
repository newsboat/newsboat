#ifndef NEWSBEUTER_FORMATSTRING__H
#define NEWSBEUTER_FORMATSTRING__H

#include <map>
#include <string>

namespace newsbeuter {

class fmtstr_formatter {
	public:
		void register_fmt(char f, const std::string& value);
		std::string do_format(const std::string& fmt, unsigned int width = 0);
		std::wstring do_wformat(const std::wstring& fmt, unsigned int width = 0);
	private:
		std::map<char, std::wstring> fmts;
};


}

#endif

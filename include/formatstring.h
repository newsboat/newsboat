#ifndef NEWSBEUTER_FORMATSTRING__H
#define NEWSBEUTER_FORMATSTRING__H

#include <map>
#include <string>

namespace newsbeuter {

class fmtstr_formatter {
	public:
		void register_fmt(char f, const std::string& value);
		std::string do_format(const std::string& fmt);
	private:
		std::map<char, std::string> fmts;
};


}

#endif

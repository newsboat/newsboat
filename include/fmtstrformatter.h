#ifndef NEWSBOAT_FORMATSTRING_H_
#define NEWSBOAT_FORMATSTRING_H_

#include "fmtstrformatter.rs.h"

#include <string>

namespace newsboat {

class FmtStrFormatter {
public:
	FmtStrFormatter();
	~FmtStrFormatter() = default;
	void register_fmt(char f, const std::string& value);
	std::string do_format(const std::string& fmt, unsigned int width = 0);

private:
	rust::Box<fmtstrformatter::bridged::FmtStrFormatter> rs_object;
};

} // namespace newsboat

#endif /* NEWSBOAT_FORMATSTRING_H_ */

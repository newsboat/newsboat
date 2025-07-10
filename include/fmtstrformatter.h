#ifndef NEWSBOAT_FORMATSTRING_H_
#define NEWSBOAT_FORMATSTRING_H_

#include "libNewsboat-ffi/src/fmtstrformatter.rs.h" // IWYU pragma: export

#include <string>

namespace Newsboat {

class FmtStrFormatter {
public:
	FmtStrFormatter();
	FmtStrFormatter(FmtStrFormatter&&) = default;
	FmtStrFormatter& operator=(FmtStrFormatter&&) = default;
	~FmtStrFormatter() = default;
	void register_fmt(char f, const std::string& value);
	std::string do_format(const std::string& fmt, unsigned int width = 0);

private:
	rust::Box<fmtstrformatter::bridged::FmtStrFormatter> rs_object;
};

} // namespace Newsboat

#endif /* NEWSBOAT_FORMATSTRING_H_ */

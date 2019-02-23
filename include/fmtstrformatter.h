#ifndef NEWSBOAT_FORMATSTRING_H_
#define NEWSBOAT_FORMATSTRING_H_

#include <map>
#include <string>

namespace newsboat {

class FmtStrFormatter {
public:
	FmtStrFormatter();
	~FmtStrFormatter();
	void register_fmt(char f, const std::string& value);
	std::string do_format(const std::string& fmt, unsigned int width = 0);

private:
	void* rs_fmt = nullptr;
};

} // namespace newsboat

#endif /* NEWSBOAT_FORMATSTRING_H_ */

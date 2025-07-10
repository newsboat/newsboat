#ifndef NEWSBOAT_RUSTSTRING_H_
#define NEWSBOAT_RUSTSTRING_H_

#include <string>

namespace Newsboat {

class RustString {
private:
	char* str;

public:
	RustString() = delete;
	RustString(const RustString&) = delete;
	RustString(RustString&& rs) = delete;
	RustString& operator=(RustString&& rs) noexcept = delete;

	explicit RustString(char* ptr);
	~RustString();

	operator std::string();
};

}

#endif /* NEWSBOAT_RUSTSTRING_H_ */

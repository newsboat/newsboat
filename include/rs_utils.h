#ifndef NEWSBOAT_RS_UTILS_H_
#define NEWSBOAT_RS_UTILS_H_

#ifdef __cplusplus
extern "C" {
#endif

char* rs_replace_all(
		const char* str,
		const char* from,
		const char* to);

char* rs_consolidate_whitespace(
		const char* str,
		const char* whitespace);

void rs_cstring_free(char* str);

class RustString {
private:
	char* str;

public:
	RustString() = delete;
	RustString(const RustString&) = delete;

	RustString(RustString&& rs):
	str(std::move(rs.str)) {
		rs.str= NULL;
	}

	RustString& operator=(RustString&& rs) {
		if ( &rs != this ) {
			str = std::move(rs.str);
			return *this;
		}
	}

	explicit RustString(char* ptr) {
		str = ptr;
	}

	operator std::string() {
		return std::string(str);
	}

	~RustString() {
		rs_cstring_free(str);
	}
};

#ifdef __cplusplus
}
#endif

#endif /* NEWSBOAT_RS_UTILS_H_ */

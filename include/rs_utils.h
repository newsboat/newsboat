#ifndef NEWSBOAT_RS_UTILS_H_
#define NEWSBOAT_RS_UTILS_H_

#ifdef __cplusplus
extern "C" {
#endif

char* rs_replace_all(
		const char* str,
		const char* from,
		const char* to);

char* rs_consolidate_whitespace(const char* str);

unsigned int rs_to_u(const char* str, const unsigned int default_value);

char* rs_trim(const char* str);

char* rs_trim_end(const char* str);

unsigned int rs_get_random_value(const unsigned int max);

void rs_cstring_free(char* str);

char* rs_get_default_browser();

bool rs_is_valid_color(const char* str);

class RustString {
private:
	char* str;

public:
	RustString() = delete;
	RustString(const RustString&) = delete;

	RustString(RustString&& rs)
		: str(std::move(rs.str))
	{
		rs.str = nullptr;
	}

	RustString& operator=(RustString&& rs) noexcept
	{
		if (&rs != this) {
			str = std::move(rs.str);
		}
		return *this;
	}

	explicit RustString(char* ptr)
	{
		str = ptr;
	}

	operator std::string()
	{
		if (str != nullptr){
			return std::string(str);
		}
		return std::string();
	}

	~RustString()
	{
		// This pointer is checked for nullptr on the rust side.
		rs_cstring_free(str);
	}
};

#ifdef __cplusplus
}
#endif

#endif /* NEWSBOAT_RS_UTILS_H_ */

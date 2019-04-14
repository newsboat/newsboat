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

char* rs_absolute_url(const char* base_url, const char* link);

char* rs_resolve_tilde(const char* str);

char* rs_resolve_relative(const char* reference, const char* fname);

bool rs_is_special_url(const char* str);

bool rs_is_http_url(const char* str);

bool rs_is_query_url(const char* str);

bool rs_is_filter_url(const char* str);

bool rs_is_exec_url(const char* str);

char* rs_censor_url(const char* str);

char* rs_trim(const char* str);

char* rs_trim_end(const char* str);

char* rs_quote(const char* str);

char* rs_quote_if_necessary(const char* str);

unsigned int rs_get_random_value(const unsigned int max);

char* rs_unescape_url(const char* str);

char* rs_make_title(const char* str);

void rs_cstring_free(char* str);

char* rs_get_default_browser();

bool rs_is_valid_color(const char* str);

bool rs_is_valid_attribute(const char* attribute);

size_t rs_strwidth(const char* str);

size_t rs_strwidth_stfl(const char* str);

bool rs_is_valid_podcast_type(const char* mimetype);

char* rs_get_command_output(const char* str);

void rs_run_command(const char* command, const char* param);

char* rs_run_program(char* argv[], const char* input);

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

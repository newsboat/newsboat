#ifndef NEWSBOAT_RS_UTILS_H_
#define NEWSBOAT_RS_UTILS_H_

#include <cstdint>

#include "3rd-party/optional.hpp"

#ifdef __cplusplus
extern "C" {
#endif

struct FilterUrl {
	char* filter;
	char* url;
};

char* rs_replace_all(
	const char* str,
	const char* from,
	const char* to);

char* rs_consolidate_whitespace(const char* str);

char* rs_absolute_url(const char* base_url, const char* link);

char* rs_resolve_tilde(const char* str);

char* rs_resolve_relative(const char* reference, const char* fname);

bool rs_is_special_url(const char* str);

bool rs_is_http_url(const char* str);

bool rs_is_query_url(const char* str);

bool rs_is_filter_url(const char* str);

bool rs_is_exec_url(const char* str);

char* rs_censor_url(const char* str);

char* rs_quote_for_stfl(const char* str);

char* rs_trim(const char* str);

char* rs_trim_end(const char* str);

char* rs_quote(const char* str);

char* rs_quote_if_necessary(const char* str);

unsigned long rs_get_auth_method(const char* str);

char* rs_unescape_url(const char* str);

char* rs_make_title(const char* str);

std::uint8_t rs_run_interactively(const char* command, const char* caller,
	bool* success);

char* rs_getcwd();

int rs_strnaturalcmp(const char* a, const char* b);

char* rs_get_default_browser();

bool rs_is_valid_color(const char* str);

bool rs_is_valid_attribute(const char* attribute);

size_t rs_strwidth(const char* str);

size_t rs_strwidth_stfl(const char* str);

char* rs_substr_with_width(const char* str, size_t max_width);

char* rs_substr_with_width_stfl(const char* str, size_t max_width);

char* rs_remove_soft_hyphens(const char* str);

bool rs_is_valid_podcast_type(const char* mimetype);

std::int64_t rs_podcast_mime_to_link_type(const char* mimetype, bool* success);

char* rs_get_command_output(const char* str);

char* rs_get_basename(const char* str);

void rs_run_command(const char* command, const char* param);

char* rs_run_program(const char* argv[], const char* input);

char* rs_program_version();

unsigned int rs_newsboat_version_major();

unsigned int rs_gentabs(const char* path);

int rs_mkdir_parents(const char* path, const std::uint32_t mode);

char* rs_strip_comments(const char* line);

FilterUrl rs_extract_filter(const char* line);

#ifdef __cplusplus
}
#endif

#endif /* NEWSBOAT_RS_UTILS_H_ */

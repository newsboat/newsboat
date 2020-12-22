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

char* rs_resolve_tilde(const char* str);

char* rs_resolve_relative(const char* reference, const char* fname);

unsigned long rs_get_auth_method(const char* str);

char* rs_unescape_url(const char* str);

std::uint8_t rs_run_interactively(const char* command, const char* caller,
	bool* success);

char* rs_getcwd();

char* rs_remove_soft_hyphens(const char* str);

bool rs_is_valid_podcast_type(const char* mimetype);

std::int64_t rs_podcast_mime_to_link_type(const char* mimetype, bool* success);

char* rs_run_program(const char* argv[], const char* input);

unsigned int rs_newsboat_version_major();

int rs_mkdir_parents(const char* path, const std::uint32_t mode);

FilterUrl rs_extract_filter(const char* line);

#ifdef __cplusplus
}
#endif

#endif /* NEWSBOAT_RS_UTILS_H_ */

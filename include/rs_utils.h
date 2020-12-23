#ifndef NEWSBOAT_RS_UTILS_H_
#define NEWSBOAT_RS_UTILS_H_

#include <cstdint>

#include "3rd-party/optional.hpp"

#ifdef __cplusplus
extern "C" {
#endif

char* rs_unescape_url(const char* str);

std::uint8_t rs_run_interactively(const char* command, const char* caller,
	bool* success);

char* rs_remove_soft_hyphens(const char* str);

bool rs_is_valid_podcast_type(const char* mimetype);

std::int64_t rs_podcast_mime_to_link_type(const char* mimetype, bool* success);

char* rs_run_program(const char* argv[], const char* input);

#ifdef __cplusplus
}
#endif

#endif /* NEWSBOAT_RS_UTILS_H_ */

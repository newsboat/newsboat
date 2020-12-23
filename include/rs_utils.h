#ifndef NEWSBOAT_RS_UTILS_H_
#define NEWSBOAT_RS_UTILS_H_

#include <cstdint>

#include "3rd-party/optional.hpp"

#ifdef __cplusplus
extern "C" {
#endif

std::int64_t rs_podcast_mime_to_link_type(const char* mimetype, bool* success);

char* rs_run_program(const char* argv[], const char* input);

#ifdef __cplusplus
}
#endif

#endif /* NEWSBOAT_RS_UTILS_H_ */

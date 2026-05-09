#ifndef NEWSBOAT_FEEDORIGIN_H_
#define NEWSBOAT_FEEDORIGIN_H_

#include <optional>

namespace newsboat {

struct FileOrigin {
	std::size_t line_number;
};

struct FeedOrigin {
	std::optional<FileOrigin> file_origin;
};

} // namespace newsboat

#endif /* NEWSBOAT_FEEDORIGIN_H_ */


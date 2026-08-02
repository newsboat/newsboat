#ifndef NEWSBOAT_FILEURLWRITER_H_
#define NEWSBOAT_FILEURLWRITER_H_

#include <optional>
#include <vector>

#include "filepath.h"
#include "urlreader.h"

namespace newsboat {

class FileUrlWriter {
public:
	static std::optional<std::string> write_urls(const std::vector<UrlReader::FeedUrl>&
		feed_urls, const Filepath& filename);
};

}

#endif /* NEWSBOAT_FILEURLWRITER_H_ */


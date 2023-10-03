#ifndef NEWSBOAT_OPML_H_
#define NEWSBOAT_OPML_H_

#include <libxml/tree.h>

#include "feedcontainer.h"
#include "fileurlreader.h"

namespace newsboat {

namespace opml {
xmlDocPtr generate(const FeedContainer& feedcontainer, bool version2);
std::optional<std::string> import(
	const Filepath& filename,
	FileUrlReader& urlcfg);
}

} // namespace newsboat

#endif /* NEWSBOAT_OPML_H_ */

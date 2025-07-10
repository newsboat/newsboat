#ifndef NEWSBOAT_OPML_H_
#define NEWSBOAT_OPML_H_

#include <libxml/tree.h>

#include "feedcontainer.h"
#include "fileurlreader.h"

namespace Newsboat {

namespace opml {
xmlDocPtr generate(const FeedContainer& feedcontainer, bool version2);
std::optional<std::string> import(
	const std::string& filename,
	FileUrlReader& urlcfg);
}

} // namespace Newsboat

#endif /* NEWSBOAT_OPML_H_ */

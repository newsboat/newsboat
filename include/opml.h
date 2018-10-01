#ifndef NEWSBOAT_OPML_H_
#define NEWSBOAT_OPML_H_

#include <libxml/tree.h>

#include "feedcontainer.h"
#include "urlreader.h"

namespace newsboat {

namespace OPML {
	xmlDocPtr generate_opml(const FeedContainer& feedcontainer);
	bool import(
			const std::string& filename,
			UrlReader* urlcfg);
}

} // namespace newsboat

#endif /* NEWSBOAT_OPML_H_ */

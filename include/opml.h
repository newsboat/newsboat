#ifndef NEWSBOAT_OPML_H_
#define NEWSBOAT_OPML_H_

#include <libxml/tree.h>

#include "feedcontainer.h"

namespace newsboat {

namespace OPML {
	xmlDocPtr prepare_opml(const FeedContainer& feedcontainer);
}

} // namespace newsboat

#endif /* NEWSBOAT_OPML_H_ */

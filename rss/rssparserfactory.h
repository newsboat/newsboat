#ifndef NEWSBOAT_RSSPP_RSSPARSERFACTORY_H_
#define NEWSBOAT_RSSPP_RSSPARSERFACTORY_H_

#include <libxml/tree.h>
#include <memory>

#include "rssparser.h"

namespace rsspp {

class Feed;

struct RssParserFactory {
	static std::shared_ptr<RssParser> get_object(Feed& f, xmlDocPtr doc);
};

} // namespace rsspp

#endif /* NEWSBOAT_RSSPP_RSSPARSERFACTORY_H_ */

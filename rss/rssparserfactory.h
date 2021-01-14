#ifndef NEWSBOAT_RSSPP_RSSPARSERFACTORY_H_
#define NEWSBOAT_RSSPP_RSSPARSERFACTORY_H_

#include <memory>
#include <libxml/tree.h>

#include "feed.h"
#include "rssparser.h"

namespace rsspp {

struct RssParserFactory {
	static std::shared_ptr<RssParser> get_object(Feed::Version rss_version,
		xmlDocPtr doc);
};

} // namespace rsspp

#endif /* NEWSBOAT_RSSPP_RSSPARSERFACTORY_H_ */

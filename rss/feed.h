#ifndef NEWSBOAT_RSSPPFEED_H_
#define NEWSBOAT_RSSPPFEED_H_

#include <string>
#include <vector>

#include "item.h"

namespace rsspp {

class Feed {
public:
	enum Version {
		UNKNOWN = 0,
		RSS_0_91,
		RSS_0_92,
		RSS_1_0,
		RSS_2_0,
		ATOM_0_3,
		ATOM_1_0,
		RSS_0_94,
		ATOM_0_3_NONS,
		TTRSS_JSON,
		NEWSBLUR_JSON,
		OCNEWS_JSON,
		MINIFLUX_JSON,
        FRESHRSS_JSON
	};

	Feed()
		: rss_version(UNKNOWN)
	{
	}

	std::string encoding;

	Version rss_version;
	std::string title;
	std::string title_type;
	std::string description;
	std::string link;
	std::string language;
	std::string managingeditor;
	std::string dc_creator;
	std::string pubDate;

	std::vector<Item> items;
};

} // namespace rsspp

#endif /* NEWSBOAT_RSSPPFEED_H_ */

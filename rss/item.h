#ifndef NEWSBOAT_RSSPPITEM_H_
#define NEWSBOAT_RSSPPITEM_H_

#include <ctime>
#include <string>
#include <vector>

namespace rsspp {

class Item {
public:
	Item()
		: guid_isPermaLink(false)
		, pubDate_ts(0)
	{}

	std::string title;
	std::string title_type;
	std::string link;
	std::string description;
	std::string description_type;

	std::string author;
	std::string author_email;

	std::string pubDate;
	std::string guid;
	bool guid_isPermaLink;

	std::string enclosure_url;
	std::string enclosure_type;

	// extensions:
	std::string content_encoded;
	std::string itunes_summary;

	// Atom-specific:
	std::string base;
	std::vector<std::string> labels;

	// only required for ttrss support:
	time_t pubDate_ts;
};

} // namespace rsspp

#endif /* NEWSBOAT_RSSPPITEM_H_ */

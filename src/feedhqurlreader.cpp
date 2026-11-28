#include "feedhqurlreader.h"

#include "config.h"
#include "configcontainer.h"

namespace newsboat {

FeedHqUrlReader::FeedHqUrlReader(ConfigContainer& c,
	const Filepath& url_file,
	RemoteApi& a)
	: RemoteApiUrlReader("FeedHQ", url_file, a)
	, cfg(c)
{
}

#define BROADCAST_FRIENDS_URL                                    \
	"http://feedhq.org/reader/atom/user/-/state/com.google/" \
	"broadcast-friends"
#define STARRED_ITEMS_URL \
	"http://feedhq.org/reader/atom/user/-/state/com.google/starred"
#define SHARED_ITEMS_URL \
	"http://feedhq.org/reader/atom/user/-/state/com.google/broadcast"

void FeedHqUrlReader::add_special_feeds()
{
	if (cfg.get_configvalue_as_bool("feedhq-show-special-feeds")) {
		feed_urls.emplace_back(FeedUrl{BROADCAST_FRIENDS_URL, FeedOrigin{}, {std::string("~") + _("People you follow")}});
		feed_urls.emplace_back(FeedUrl{STARRED_ITEMS_URL, FeedOrigin{}, {std::string("~") + _("Starred items")}});
		feed_urls.emplace_back(FeedUrl{SHARED_ITEMS_URL, FeedOrigin{}, {std::string("~") + _("Shared items")}});
	}
}

} // namespace newsboat

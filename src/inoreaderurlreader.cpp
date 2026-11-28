#include "inoreaderurlreader.h"

#include "config.h"
#include "configcontainer.h"

namespace newsboat {

InoreaderUrlReader::InoreaderUrlReader(ConfigContainer& c,
	const Filepath& url_file,
	RemoteApi& a)
	: RemoteApiUrlReader("inoreader", url_file, a)
	, cfg(c)
{
}

#define STARRED_ITEMS_URL \
	"http://inoreader.com/reader/atom/user/-/state/com.google/starred"
#define BROADCAST_ITEMS_URL \
	"http://inoreader.com/reader/atom/user/-/state/com.google/broadcast"
#define LIKED_ITEMS_URL \
	"http://inoreader.com/reader/atom/user/-/state/com.google/like"
#define SAVED_WEB_PAGES_ITEMS_URL                                   \
	"http://inoreader.com/reader/atom/user/-/state/com.google/" \
	"saved-web-pages"

void InoreaderUrlReader::add_special_feeds()
{
	if (cfg.get_configvalue_as_bool("inoreader-show-special-feeds")) {
		feed_urls.emplace_back(FeedUrl{STARRED_ITEMS_URL, FeedOrigin{}, {std::string("~") + _("Starred items")}});
		feed_urls.emplace_back(FeedUrl{BROADCAST_ITEMS_URL, FeedOrigin{}, {std::string("~") + _("Broadcast items")}});
		feed_urls.emplace_back(FeedUrl{LIKED_ITEMS_URL, FeedOrigin{}, {std::string("~") + _("Liked items")}});
		feed_urls.emplace_back(FeedUrl{SAVED_WEB_PAGES_ITEMS_URL, FeedOrigin{}, {std::string("~") + _("Saved web pages")}});
	}
}

} // namespace newsboat

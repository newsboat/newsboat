#include "oldreaderurlreader.h"

#include "config.h"
#include "configcontainer.h"

namespace newsboat {

OldReaderUrlReader::OldReaderUrlReader(ConfigContainer& c,
	const Filepath& url_file,
	RemoteApi& a)
	: RemoteApiUrlReader("The Old Reader", url_file, a)
	, cfg(c)
{
}

#define BROADCAST_FRIENDS_URL                                           \
	"https://theoldreader.com/reader/atom/user/-/state/com.google/" \
	"broadcast-friends"
#define STARRED_ITEMS_URL \
	"https://theoldreader.com/reader/atom/user/-/state/com.google/starred"
#define SHARED_ITEMS_URL                                                \
	"https://theoldreader.com/reader/atom/user/-/state/com.google/" \
	"broadcast"

void OldReaderUrlReader::add_special_feeds()
{
	if (cfg.get_configvalue_as_bool("oldreader-show-special-feeds")) {
		feed_urls.emplace_back(FeedUrl{BROADCAST_FRIENDS_URL, FeedOrigin{}, {std::string("~") + _("People you follow")}});
		feed_urls.emplace_back(FeedUrl{STARRED_ITEMS_URL, FeedOrigin{}, {std::string("~") + _("Starred items")}});
		feed_urls.emplace_back(FeedUrl{SHARED_ITEMS_URL, FeedOrigin{}, {std::string("~") + _("Shared items")}});
	}
}

} // namespace newsboat

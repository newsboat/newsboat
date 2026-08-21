#include "oldreaderurlreader.h"

#include "config.h"
#include "configcontainer.h"
#include "logger.h"
#include "remoteapi.h"
#include "utils.h"

namespace newsboat {

OldReaderUrlReader::OldReaderUrlReader(ConfigContainer* c,
	const Filepath& url_file,
	RemoteApi* a)
	: cfg(c)
	, file(url_file)
	, api(a)
{
}

OldReaderUrlReader::~OldReaderUrlReader() {}

#define BROADCAST_FRIENDS_URL                                           \
	"https://theoldreader.com/reader/atom/user/-/state/com.google/" \
	"broadcast-friends"
#define STARRED_ITEMS_URL \
	"https://theoldreader.com/reader/atom/user/-/state/com.google/starred"
#define SHARED_ITEMS_URL                                                \
	"https://theoldreader.com/reader/atom/user/-/state/com.google/" \
	"broadcast"

std::optional<utils::ReadTextFileError> OldReaderUrlReader::reload()
{
	feed_urls.clear();

	if (cfg->get_configvalue_as_bool("oldreader-show-special-feeds")) {
		feed_urls.emplace_back(FeedUrl{BROADCAST_FRIENDS_URL, FeedOrigin{}, {std::string("~") + _("People you follow")}});
		feed_urls.emplace_back(FeedUrl{STARRED_ITEMS_URL, FeedOrigin{}, {std::string("~") + _("Starred items")}});
		feed_urls.emplace_back(FeedUrl{SHARED_ITEMS_URL, FeedOrigin{}, {std::string("~") + _("Shared items")}});
	}

	load_query_urls_from_file(file);

	std::vector<TaggedFeedUrl> feedurls = api->get_subscribed_urls();
	for (const auto& url : feedurls) {
		LOG(Level::DEBUG, "added %s to URL list", url.first);
		feed_urls.emplace_back(FeedUrl{url.first, FeedOrigin{}, url.second});
		for (const auto& tag : url.second) {
			LOG(Level::DEBUG, "%s: added tag %s", url.first, tag);
		}
	}

	return {};
}

std::string OldReaderUrlReader::get_source() const
{
	return "The Old Reader";
}

} // namespace newsboat

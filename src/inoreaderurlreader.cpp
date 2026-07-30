#include "inoreaderurlreader.h"

#include "config.h"
#include "configcontainer.h"
#include "logger.h"
#include "remoteapi.h"
#include "utils.h"

namespace newsboat {

InoreaderUrlReader::InoreaderUrlReader(ConfigContainer* c,
	const Filepath& url_file,
	RemoteApi* a)
	: cfg(c)
	, file(url_file)
	, api(a)
{
}

InoreaderUrlReader::~InoreaderUrlReader() {}

#define STARRED_ITEMS_URL \
	"http://inoreader.com/reader/atom/user/-/state/com.google/starred"
#define BROADCAST_ITEMS_URL \
	"http://inoreader.com/reader/atom/user/-/state/com.google/broadcast"
#define LIKED_ITEMS_URL \
	"http://inoreader.com/reader/atom/user/-/state/com.google/like"
#define SAVED_WEB_PAGES_ITEMS_URL                                   \
	"http://inoreader.com/reader/atom/user/-/state/com.google/" \
	"saved-web-pages"

std::optional<utils::ReadTextFileError> InoreaderUrlReader::reload()
{
	feed_urls.clear();

	if (cfg->get_configvalue_as_bool("inoreader-show-special-feeds")) {
		feed_urls.emplace_back(FeedUrl{STARRED_ITEMS_URL, FeedOrigin{}, {std::string("~") + _("Starred items")}});
		feed_urls.emplace_back(FeedUrl{BROADCAST_ITEMS_URL, FeedOrigin{}, {std::string("~") + _("Broadcast items")}});
		feed_urls.emplace_back(FeedUrl{LIKED_ITEMS_URL, FeedOrigin{}, {std::string("~") + _("Liked items")}});
		feed_urls.emplace_back(FeedUrl{SAVED_WEB_PAGES_ITEMS_URL, FeedOrigin{}, {std::string("~") + _("Saved web pages")}});
	}

	load_query_urls_from_file(file);

	std::vector<TaggedFeedUrl> subscribed_urls = api->get_subscribed_urls();
	for (const auto& url : subscribed_urls) {
		LOG(Level::DEBUG, "added %s to URL list", url.first);
		feed_urls.push_back(FeedUrl{url.first, FeedOrigin{}, url.second});
		for (const auto& tag : url.second) {
			LOG(Level::DEBUG, "%s: added tag %s", url.first, tag);
		}
	}

	return {};
}

std::string InoreaderUrlReader::get_source() const
{
	return "inoreader";
}

} // namespace newsboat

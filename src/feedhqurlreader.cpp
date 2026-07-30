#include "feedhqurlreader.h"

#include "config.h"
#include "configcontainer.h"
#include "logger.h"
#include "remoteapi.h"
#include "utils.h"

namespace newsboat {

FeedHqUrlReader::FeedHqUrlReader(ConfigContainer* c,
	const Filepath& url_file,
	RemoteApi* a)
	: cfg(c)
	, file(url_file)
	, api(a)
{
}

FeedHqUrlReader::~FeedHqUrlReader() {}

#define BROADCAST_FRIENDS_URL                                    \
	"http://feedhq.org/reader/atom/user/-/state/com.google/" \
	"broadcast-friends"
#define STARRED_ITEMS_URL \
	"http://feedhq.org/reader/atom/user/-/state/com.google/starred"
#define SHARED_ITEMS_URL \
	"http://feedhq.org/reader/atom/user/-/state/com.google/broadcast"

std::optional<utils::ReadTextFileError> FeedHqUrlReader::reload()
{
	feed_urls.clear();

	if (cfg->get_configvalue_as_bool("feedhq-show-special-feeds")) {
		feed_urls.emplace_back(FeedUrl{BROADCAST_FRIENDS_URL, FeedOrigin{}, {std::string("~") + _("People you follow")}});
		feed_urls.emplace_back(FeedUrl{STARRED_ITEMS_URL, FeedOrigin{}, {std::string("~") + _("Starred items")}});
		feed_urls.emplace_back(FeedUrl{SHARED_ITEMS_URL, FeedOrigin{}, {std::string("~") + _("Shared items")}});
	}

	load_query_urls_from_file(file);

	std::vector<TaggedFeedUrl> subscribed_urls = api->get_subscribed_urls();
	for (const auto& tagged : subscribed_urls) {
		std::string url = tagged.first;
		std::vector<std::string> url_tags = tagged.second;

		LOG(Level::DEBUG, "added %s to URL list", url);
		feed_urls.emplace_back(FeedUrl{url, FeedOrigin{}, url_tags});
		for (const auto& tag : url_tags) {
			LOG(Level::DEBUG, "%s: added tag %s", url, tag);
		}
	}

	return {};
}

std::string FeedHqUrlReader::get_source() const
{
	return "FeedHQ";
}

} // namespace newsboat

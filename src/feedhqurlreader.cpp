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

#define ADD_URL(url, caption)                 \
	do {                                  \
		tmptags.clear();              \
		urls.push_back((url));        \
		tmptags.push_back((caption)); \
		tags[(url)] = tmptags;        \
	} while (0)

std::optional<utils::ReadTextFileError> FeedHqUrlReader::reload()
{
	urls.clear();
	tags.clear();

	if (cfg->get_configvalue_as_bool("feedhq-show-special-feeds")) {
		std::vector<std::string> tmptags;
		ADD_URL(BROADCAST_FRIENDS_URL,
			std::string("~") + _("People you follow"));
		ADD_URL(STARRED_ITEMS_URL,
			std::string("~") + _("Starred items"));
		ADD_URL(SHARED_ITEMS_URL, std::string("~") + _("Shared items"));
	}

	load_query_urls_from_file(file);

	std::vector<TaggedFeedUrl> feedurls = api->get_subscribed_urls();
	for (const auto& tagged : feedurls) {
		std::string url = tagged.first;
		std::vector<std::string> url_tags = tagged.second;

		LOG(Level::DEBUG, "added %s to URL list", url);
		urls.push_back(url);
		tags[tagged.first] = url_tags;
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

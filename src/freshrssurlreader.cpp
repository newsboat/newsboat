#include "freshrssurlreader.h"

#include "config.h"
#include "configcontainer.h"
#include "logger.h"
#include "remoteapi.h"
#include "utils.h"

namespace newsboat {

FreshRssUrlReader::FreshRssUrlReader(ConfigContainer* c,
	const Filepath& url_file,
	RemoteApi* a)
	: cfg(c)
	, file(url_file)
	, api(a)
{
}

FreshRssUrlReader::~FreshRssUrlReader() {}

std::optional<utils::ReadTextFileError> FreshRssUrlReader::reload()
{
	feed_urls.clear();

	if (cfg->get_configvalue_as_bool("freshrss-show-special-feeds")) {
		const std::string star_url = cfg->get_configvalue("freshrss-url") +
			"/reader/api/0/stream/contents/user/-/state/com.google/starred";
		feed_urls.emplace_back(FeedUrl{star_url, FeedOrigin{}, {std::string("~") + _("Starred items")}});
	}

	load_query_urls_from_file(file);

	std::vector<TaggedFeedUrl> feedurls = api->get_subscribed_urls();
	for (const auto& tagged : feedurls) {
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

std::string FreshRssUrlReader::get_source() const
{
	return "FreshRSS";
}

} // namespace newsboat

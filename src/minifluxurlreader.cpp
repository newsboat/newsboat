#include "minifluxurlreader.h"

#include "config.h"
#include "configcontainer.h"
#include "logger.h"
#include "remoteapi.h"
#include "utils.h"

namespace newsboat {

MinifluxUrlReader::MinifluxUrlReader(ConfigContainer* c,
	const Filepath& url_file,
	RemoteApi* a)
	: cfg(c)
	, file(url_file)
	, api(a)
{
}

MinifluxUrlReader::~MinifluxUrlReader() {}

std::optional<utils::ReadTextFileError> MinifluxUrlReader::reload()
{
	feed_urls.clear();

	if (cfg->get_configvalue_as_bool("miniflux-show-special-feeds")) {
		feed_urls.emplace_back(FeedUrl{"starred", FeedOrigin{}, {std::string("~") + _("Starred items")}});
	}

	load_query_urls_from_file(file);

	const std::vector<TaggedFeedUrl> subscribed_urls = api->get_subscribed_urls();

	for (const auto& url : subscribed_urls) {
		LOG(Level::INFO, "added %s to URL list", url.first);
		feed_urls.emplace_back(FeedUrl{url.first, FeedOrigin{}, url.second});
		for (const auto& tag : url.second) {
			LOG(Level::DEBUG, "%s: added tag %s", url.first, tag);
		}
	}

	return {};
}

std::string MinifluxUrlReader::get_source() const
{
	return "Miniflux";
}

} // namespace newsboat

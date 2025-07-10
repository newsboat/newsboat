#include "freshrssurlreader.h"

#include "config.h"
#include "configcontainer.h"
#include "logger.h"
#include "remoteapi.h"
#include "utils.h"

namespace Newsboat {

FreshRssUrlReader::FreshRssUrlReader(ConfigContainer* c,
	const std::string& url_file,
	RemoteApi* a)
	: cfg(c)
	, file(url_file)
	, api(a)
{
}

FreshRssUrlReader::~FreshRssUrlReader() {}

std::optional<utils::ReadTextFileError> FreshRssUrlReader::reload()
{
	urls.clear();
	tags.clear();

	if (cfg->get_configvalue_as_bool("freshrss-show-special-feeds")) {
		std::vector<std::string> tmptags;
		const std::string star_url = cfg->get_configvalue("freshrss-url") +
			"/reader/api/0/stream/contents/user/-/state/com.google/starred";
		urls.push_back(star_url);
		tmptags.push_back(std::string("~") + _("Starred items"));
		tags[star_url] = tmptags;
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

std::string FreshRssUrlReader::get_source() const
{
	return "FreshRSS";
}

} // namespace Newsboat

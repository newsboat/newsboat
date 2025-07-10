#include "minifluxurlreader.h"

#include "config.h"
#include "configcontainer.h"
#include "logger.h"
#include "remoteapi.h"
#include "utils.h"

namespace Newsboat {

MinifluxUrlReader::MinifluxUrlReader(ConfigContainer* c,
	const std::string& url_file,
	RemoteApi* a)
	: cfg(c)
	, file(url_file)
	, api(a)
{
}

MinifluxUrlReader::~MinifluxUrlReader() {}

std::optional<utils::ReadTextFileError> MinifluxUrlReader::reload()
{
	urls.clear();
	tags.clear();

	if (cfg->get_configvalue_as_bool("miniflux-show-special-feeds")) {
		std::vector<std::string> tmptags;
		const std::string star_url = "starred";
		urls.push_back(star_url);
		std::string star_tag = std::string("~") + _("Starred items");
		tmptags.push_back(star_tag);
		tags[star_url] = tmptags;
	}

	load_query_urls_from_file(file);

	const std::vector<TaggedFeedUrl> feedurls = api->get_subscribed_urls();

	for (const auto& url : feedurls) {
		LOG(Level::INFO, "added %s to URL list", url.first);
		urls.push_back(url.first);
		tags[url.first] = url.second;
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

} // namespace Newsboat

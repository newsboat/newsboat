#include "freshrssurlreader.h"

#include "config.h"
#include "configcontainer.h"
#include "fileurlreader.h"
#include "logger.h"
#include "remoteapi.h"
#include "utils.h"

namespace newsboat {

FreshRssUrlReader::FreshRssUrlReader(ConfigContainer* c,
	const Filepath& url_file,
	RemoteApi* a)
	: cfg(c)
	, file(url_file.clone())
	, api(a)
{
}

FreshRssUrlReader::~FreshRssUrlReader() {}

std::optional<utils::ReadTextFileError> FreshRssUrlReader::reload()
{
	urls.clear();
	tags.clear();
	alltags.clear();

	if (cfg->get_configvalue_as_bool("freshrss-show-special-feeds")) {
		std::vector<std::string> tmptags;
		const std::string star_url = cfg->get_configvalue("freshrss-url") +
			"/reader/api/0/stream/contents/user/-/state/com.google/starred";
		urls.push_back(star_url);
		tmptags.push_back(std::string("~") + _("Starred items"));
		tags[star_url] = tmptags;
	}

	FileUrlReader ur(file);
	const auto error_message = ur.reload();
	if (error_message.has_value()) {
		LOG(Level::DEBUG, "Reloading failed: %s", error_message.value().message);
		// Ignore errors for now: https://github.com/newsboat/newsboat/issues/1273
	}

	for (const auto& url : ur.get_urls()) {
		if (utils::is_query_url(url)) {
			urls.push_back(url);

			auto url_tags = ur.get_tags(url);
			tags[url] = url_tags;
			for (const auto& tag : url_tags) {
				alltags.insert(tag);
			}
		}
	}

	std::vector<TaggedFeedUrl> feedurls = api->get_subscribed_urls();
	for (const auto& tagged : feedurls) {
		std::string url = tagged.first;
		std::vector<std::string> url_tags = tagged.second;

		LOG(Level::DEBUG, "added %s to URL list", url);
		urls.push_back(url);
		tags[tagged.first] = url_tags;
		for (const auto& tag : url_tags) {
			LOG(Level::DEBUG, "%s: added tag %s", url, tag);
			alltags.insert(tag);
		}
	}

	return {};
}

std::string FreshRssUrlReader::get_source()
{
	return "FreshRSS";
}

} // namespace newsboat

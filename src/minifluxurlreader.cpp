#include "minifluxurlreader.h"

#include "config.h"
#include "configcontainer.h"
#include "fileurlreader.h"
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
	urls.clear();
	tags.clear();
	alltags.clear();

	if (cfg->get_configvalue_as_bool("miniflux-show-special-feeds")) {
		std::vector<std::string> tmptags;
		const std::string star_url = "starred";
		urls.push_back(star_url);
		std::string star_tag = std::string("~") + _("Starred items");
		tmptags.push_back(star_tag);
		alltags.insert(star_tag);
		tags[star_url] = tmptags;
	}

	FileUrlReader ur(file);
	const auto error_message = ur.reload();
	if (error_message.has_value()) {
		LOG(Level::DEBUG, "Reloading failed: %s", error_message.value().message);
		// Ignore errors for now: https://github.com/newsboat/newsboat/issues/1273
	}

	const std::vector<std::string>& file_urls(ur.get_urls());
	for (const auto& url : file_urls) {
		if (utils::is_query_url(url)) {
			urls.push_back(url);
		}
	}

	const std::vector<TaggedFeedUrl> feedurls = api->get_subscribed_urls();

	for (const auto& url : feedurls) {
		LOG(Level::INFO, "added %s to URL list", url.first);
		urls.push_back(url.first);
		tags[url.first] = url.second;
		for (const auto& tag : url.second) {
			LOG(Level::DEBUG, "%s: added tag %s", url.first, tag);
			alltags.insert(tag);
		}
	}

	return {};
}

std::string MinifluxUrlReader::get_source()
{
	return "Miniflux";
}

} // namespace newsboat

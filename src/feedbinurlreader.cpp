#include "feedbinurlreader.h"

#include "configcontainer.h"
#include "fileurlreader.h"
#include "logger.h"
#include "remoteapi.h"
#include "utils.h"

namespace newsboat {

FeedbinUrlReader::FeedbinUrlReader(ConfigContainer* c,
	const std::string& url_file,
	RemoteApi* a)
	: cfg(c)
	, file(url_file)
	, api(a)
{
}

FeedbinUrlReader::~FeedbinUrlReader() {}

nonstd::optional<utils::ReadTextFileError> FeedbinUrlReader::reload()
{
	urls.clear();
	tags.clear();
	alltags.clear();

	FileUrlReader ur(file);
	const auto error_message = ur.reload();
	if (error_message.has_value()) {
		LOG(Level::DEBUG, "Reloading failed: %s", error_message.value().message);
		// Ignore errors for now: https://github.com/newsboat/newsboat/issues/1273
	}

	const std::vector<std::string>& file_urls(ur.get_urls());
	LOG(Level::INFO, "URL count: %s", file_urls.size());
	for (const auto& url : file_urls) {
		LOG(Level::INFO, "It's a URL: %s", url);
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

std::string FeedbinUrlReader::get_source()
{
	return "Feedbin";
}

} // namespace newsboat

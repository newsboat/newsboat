#include "ttrssurlreader.h"

#include "fileurlreader.h"
#include "logger.h"
#include "remoteapi.h"
#include "utils.h"

namespace newsboat {

TtRssUrlReader::TtRssUrlReader(const std::string& url_file, RemoteApi* a)
	: file(url_file)
	, api(a)
{
}

TtRssUrlReader::~TtRssUrlReader() {}

std::optional<utils::ReadTextFileError> TtRssUrlReader::reload()
{
	urls.clear();
	tags.clear();

	FileUrlReader ur(file);
	const auto error_message = ur.reload();
	if (error_message.has_value()) {
		LOG(Level::DEBUG, "Reloading failed: %s", error_message.value().message);
		// Ignore errors for now: https://github.com/newsboat/newsboat/issues/1273
	}

	auto& file_urls(ur.get_urls());
	for (const auto& url : file_urls) {
		if (utils::is_query_url(url)) {
			urls.push_back(url);
			tags[url] = ur.get_tags(url);
		}
	}

	auto feedurls = api->get_subscribed_urls();

	for (const auto& url : feedurls) {
		LOG(Level::DEBUG, "added %s to URL list", url.first);
		urls.push_back(url.first);
		tags[url.first] = url.second;
		for (const auto& tag : url.second) {
			LOG(Level::DEBUG, "%s: added tag %s", url.first, tag);
		}
	}

	return {};
}

std::string TtRssUrlReader::get_source() const
{
	return "Tiny Tiny RSS";
}

} // namespace newsboat

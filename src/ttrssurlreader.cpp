#include "ttrssurlreader.h"

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

	load_query_urls_from_file(file);

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

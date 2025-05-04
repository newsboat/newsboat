#include "feedbinurlreader.h"

#include "logger.h"
#include "remoteapi.h"
#include "utils.h"

namespace newsboat {

FeedbinUrlReader::FeedbinUrlReader(const std::string& url_file, RemoteApi* a)
	: file(url_file)
	, api(a)
{
}

std::optional<utils::ReadTextFileError> FeedbinUrlReader::reload()
{
	urls.clear();
	tags.clear();

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

std::string FeedbinUrlReader::get_source() const
{
	return "Feedbin";
}

} // namespace newsboat

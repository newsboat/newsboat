#include "newsblururlreader.h"

#include "logger.h"
#include "remoteapi.h"
#include "utils.h"

namespace newsboat {

NewsBlurUrlReader::NewsBlurUrlReader(const std::string& url_file,
	RemoteApi* a)
	: file(url_file)
	, api(a)
{
}

NewsBlurUrlReader::~NewsBlurUrlReader() {}

std::optional<utils::ReadTextFileError> NewsBlurUrlReader::reload()
{
	urls.clear();
	tags.clear();

	load_query_urls_from_file(file);

	std::vector<TaggedFeedUrl> feedurls = api->get_subscribed_urls();

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

std::string NewsBlurUrlReader::get_source() const
{
	return "NewsBlur";
}

} // namespace newsboat

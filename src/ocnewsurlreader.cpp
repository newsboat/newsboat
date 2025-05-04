#include "ocnewsurlreader.h"

#include "logger.h"
#include "remoteapi.h"
#include "utils.h"

namespace newsboat {

OcNewsUrlReader::OcNewsUrlReader(const std::string& url_file, RemoteApi* a)
	: file(url_file)
	, api(a)
{
}

OcNewsUrlReader::~OcNewsUrlReader() {}

std::optional<utils::ReadTextFileError> OcNewsUrlReader::reload()
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

std::string OcNewsUrlReader::get_source() const
{
	return "ownCloud News";
}

} // namespace newsboat

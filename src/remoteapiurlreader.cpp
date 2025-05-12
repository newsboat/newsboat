#include "remoteapiurlreader.h"

#include "logger.h"
#include "remoteapi.h"
#include "utils.h"

namespace newsboat {

RemoteApiUrlReader::RemoteApiUrlReader(const std::string& source_name,
	const Filepath& url_file, RemoteApi& api)
	: source_name(source_name)
	, file(url_file)
	, api(api)
{
}

std::optional<utils::ReadTextFileError> RemoteApiUrlReader::reload()
{
	urls.clear();
	tags.clear();

	load_query_urls_from_file(file);

	const std::vector<TaggedFeedUrl> feedurls = api.get_subscribed_urls();

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

std::string RemoteApiUrlReader::get_source() const
{
	return source_name;
}

} // namespace newsboat

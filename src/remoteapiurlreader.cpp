#include "remoteapiurlreader.h"

#include "logger.h"
#include "remoteapi.h"
#include "utils.h"
#include "config.h"

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
		if (std::find(urls.begin(), urls.end(), url.first) == urls.end()) {
			LOG(Level::INFO, "added %s to URL list", url.first);
			urls.push_back(url.first);
			tags[url.first] = url.second;
			for (const auto& tag : url.second) {
				LOG(Level::DEBUG, "%s: added tag %s", url.first, tag);
			}
		} else {
			std::string warn_msg = strprintf::fmt(
					_("Warning: Duplicate URL found: %s. Merging tags."),
					url.first);

			LOG(Level::USERERROR, warn_msg.c_str());

			for (const auto& tag : url.second) {
				if (std::find(tags[url.first].begin(),
						tags[url.first].end(),
						tag) == tags[url.first].end()) {
					LOG(Level::DEBUG, "%s: added tag %s", url.first, tag);
					tags[url.first].push_back(tag);
				}
			}
		}
	}

	return {};
}

std::string RemoteApiUrlReader::get_source() const
{
	return source_name;
}

} // namespace newsboat

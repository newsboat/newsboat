#include "remoteapiurlreader.h"

#include "logger.h"
#include "remoteapi.h"
#include "utils.h"
#include "config.h"

#include <iostream>

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
	feed_urls.clear();

	load_query_urls_from_file(file);

	const std::vector<TaggedFeedUrl> subsribed_urls = api.get_subscribed_urls();

	for (const auto& url : subsribed_urls) {
		auto it = std::find_if(feed_urls.begin(), feed_urls.end(),
		[&url](const FeedUrl& u) {
			return u.url == url.first;
		});
		if (it == feed_urls.end()) {
			LOG(Level::INFO, "added %s to URL list", url.first);
			feed_urls.emplace_back(FeedUrl{url.first, FeedOrigin{}, url.second});
			for (const auto& tag : url.second) {
				LOG(Level::DEBUG, "%s: added tag %s", url.first, tag);
			}
		} else {
			std::string warn_msg = strprintf::fmt(
					_("Warning: Duplicate URL found: %s. Merging tags."),
					url.first);

			LOG(Level::USERERROR, warn_msg.c_str());
			std::cerr << warn_msg << std::endl;

			for (const auto& tag : url.second) {
				if (std::find(it->tags.begin(), it->tags.end(), tag) == it->tags.end()) {
					LOG(Level::DEBUG, "%s: added tag %s", url.first, tag);
					it->tags.push_back(tag);
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

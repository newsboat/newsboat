#include "urlreader.h"

#include <set>

#include "fileurlreader.h"
#include "logger.h"

namespace newsboat {

const std::vector<UrlReader::FeedUrl>& UrlReader::get_urls() const
{
	return feed_urls;
}

UrlReader::FeedUrl* UrlReader::get_entry(const std::string& url)
{
	auto it = std::find_if(feed_urls.begin(),
	feed_urls.end(), [&url](const FeedUrl& feed_url) {
		return feed_url.url == url;
	});
	if (it != feed_urls.end()) {
		return &*it;
	} else {
		return nullptr;
	}
}

std::vector<std::string> UrlReader::get_alltags() const
{
	std::set<std::string> tmptags;
	for (const auto& feed : feed_urls) {
		for (const auto& tag : feed.tags) {
			if (tag.substr(0, 1) != "~") {
				// std::copy_if would make this code less readable IMHO
				// cppcheck-suppress useStlAlgorithm
				tmptags.insert(tag);
			}
		}
	}
	return std::vector<std::string>(tmptags.begin(), tmptags.end());
}

void UrlReader::load_query_urls_from_file(Filepath file)
{
	FileUrlReader file_url_reader(file);
	const auto error_message = file_url_reader.reload();
	if (error_message.has_value()) {
		LOG(Level::DEBUG, "Reloading failed: %s", error_message.value().message);
		// Ignore errors for now: https://github.com/newsboat/newsboat/issues/1273
	}

	const auto& other_urls = file_url_reader.get_urls();
	for (const auto& feed_url : other_urls) {
		if (utils::is_query_url(feed_url.url)) {
			feed_urls.push_back(feed_url);
		}
	}
}

} // namespace newsboat

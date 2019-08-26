#include "feedlyurlreader.h"

#include "configcontainer.h"
#include "fileurlreader.h"
#include "remoteapi.h"
#include "utils.h"

namespace newsboat {

FeedlyUrlReader::FeedlyUrlReader(ConfigContainer* c,
	const std::string& url_file,
	RemoteApi* a)
	: cfg(c)
	, file(url_file)
	, api(a)
{}

FeedlyUrlReader::~FeedlyUrlReader() {}

void FeedlyUrlReader::reload()
{
	urls.clear();
	tags.clear();
	alltags.clear();

	FileUrlReader ur(file);
	ur.reload();

	std::vector<std::string>& file_urls(ur.get_urls());
	for (const auto& url : file_urls) {
		if (utils::is_query_url(url)) {
			urls.push_back(url);
		}
	}

	std::vector<TaggedFeedUrl> feedurls = api->get_subscribed_urls();

	for (const auto& url : feedurls) {
		LOG(Level::INFO, "added %s to URL list", url.first);
		urls.push_back(url.first);
		tags[url.first] = url.second;
		for (const auto& tag : url.second) {
			LOG(Level::DEBUG, "%s: added tag %s", url.first, tag);
			alltags.insert(tag);
		}
	}
}

std::string FeedlyUrlReader::get_source()
{
	return "Feedly";
}

}

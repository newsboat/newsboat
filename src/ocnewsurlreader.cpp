#include "ocnewsapi.h"

#include "fileurlreader.h"
#include "logger.h"
#include "utils.h"

namespace newsboat {

OcNewsUrlReader::OcNewsUrlReader(const std::string& url_file, RemoteApi* a)
	: file(url_file)
	, api(a)
{
}

OcNewsUrlReader::~OcNewsUrlReader() {}

void OcNewsUrlReader::write_config()
{
	// NOTHING
}

void OcNewsUrlReader::reload()
{
	urls.clear();
	tags.clear();
	alltags.clear();

	FileUrlReader ur(file);
	ur.reload();

	std::vector<std::string>& file_urls(ur.get_urls());
	for (const auto& url : file_urls) {
		if (Utils::is_query_url(url)) {
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

std::string OcNewsUrlReader::get_source()
{
	return "ownCloud News";
}

} // namespace newsboat

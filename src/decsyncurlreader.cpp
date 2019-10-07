#include "decsyncapi.h"

#include "fileurlreader.h"
#include "logger.h"
#include "utils.h"

namespace newsboat {

DecsyncUrlReader::DecsyncUrlReader(const std::string& url_file,
	RemoteApi* a)
	: file(url_file)
	, api(a)
{
}

void DecsyncUrlReader::write_config()
{
	// NOTHING
}

void DecsyncUrlReader::reload()
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

std::string DecsyncUrlReader::get_source()
{
	return "DecSync";
}

}

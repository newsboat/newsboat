#include "newsblurapi.h"

#include "fileurlreader.h"
#include "logger.h"
#include "utils.h"

namespace newsboat {

NewsBlurUrlReader::NewsBlurUrlReader(const std::string& url_file,
	RemoteApi* a)
	: file(url_file)
	, api(a)
{
}

NewsBlurUrlReader::~NewsBlurUrlReader() {}

void NewsBlurUrlReader::write_config()
{
	// NOTHING
}

void NewsBlurUrlReader::reload()
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

	std::vector<tagged_feedurl> feedurls = api->get_subscribed_urls();

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

std::string NewsBlurUrlReader::get_source()
{
	return "NewsBlur";
}
// TODO

} // namespace newsboat

#include "ttrssurlreader.h"

#include "fileurlreader.h"
#include "logger.h"
#include "remoteapi.h"
#include "utils.h"

namespace newsboat {

TtRssUrlReader::TtRssUrlReader(const std::string& url_file, RemoteApi* a)
	: file(url_file)
	, api(a)
{}

TtRssUrlReader::~TtRssUrlReader() {}

void TtRssUrlReader::write_config()
{
	// NOTHING
}

void TtRssUrlReader::reload()
{
	urls.clear();
	tags.clear();
	alltags.clear();

	FileUrlReader ur(file);
	ur.reload();

	// std::vector<std::string>& file_urls(ur.get_urls());
	auto& file_urls(ur.get_urls());
	for (const auto& url : file_urls) {
		if (utils::is_query_url(url)) {
			urls.push_back(url);
			std::vector<std::string>& file_tags(ur.get_tags(url));
			tags[url] = ur.get_tags(url);
			for (const auto& tag : file_tags) {
				alltags.insert(tag);
			}
		}
	}

	auto feedurls = api->get_subscribed_urls();

	for (const auto& url : feedurls) {
		LOG(Level::DEBUG, "added %s to URL list", url.first);
		urls.push_back(url.first);
		tags[url.first] = url.second;
		for (const auto& tag : url.second) {
			LOG(Level::DEBUG, "%s: added tag %s", url.first, tag);
			alltags.insert(tag);
		}
	}
}

std::string TtRssUrlReader::get_source()
{
	return "Tiny Tiny RSS";
}
// TODO

} // namespace newsboat

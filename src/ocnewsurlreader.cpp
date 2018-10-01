#include "ocnewsapi.h"

#include "fileurlreader.h"
#include "logger.h"
#include "utils.h"

namespace newsboat {

ocnews_urlreader::ocnews_urlreader(const std::string& url_file, remote_api* a)
	: file(url_file)
	, api(a)
{
}

ocnews_urlreader::~ocnews_urlreader() {}

void ocnews_urlreader::write_config()
{
	// NOTHING
}

void ocnews_urlreader::reload()
{
	urls.clear();
	tags.clear();
	alltags.clear();

	file_urlreader ur(file);
	ur.reload();

	std::vector<std::string>& file_urls(ur.get_urls());
	for (const auto& url : file_urls) {
		if (utils::is_query_url(url)) {
			urls.push_back(url);
		}
	}

	std::vector<tagged_feedurl> feedurls = api->get_subscribed_urls();

	for (const auto& url : feedurls) {
		LOG(level::INFO, "added %s to URL list", url.first);
		urls.push_back(url.first);
		tags[url.first] = url.second;
		for (const auto& tag : url.second) {
			LOG(level::DEBUG, "%s: added tag %s", url.first, tag);
			alltags.insert(tag);
		}
	}
}

std::string ocnews_urlreader::get_source()
{
	return "ownCloud News";
}

} // namespace newsboat

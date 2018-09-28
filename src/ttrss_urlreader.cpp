#include "ttrss_api.h"

#include "file_urlreader.h"
#include "logger.h"

namespace newsboat {

ttrss_urlreader::ttrss_urlreader(const std::string& url_file, remote_api* a)
	: file(url_file)
	, api(a)
{
}

ttrss_urlreader::~ttrss_urlreader() {}

void ttrss_urlreader::write_config()
{
	// NOTHING
}

void ttrss_urlreader::reload()
{
	urls.clear();
	tags.clear();
	alltags.clear();

	file_urlreader ur(file);
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
		LOG(level::DEBUG, "added %s to URL list", url.first);
		urls.push_back(url.first);
		tags[url.first] = url.second;
		for (const auto& tag : url.second) {
			LOG(level::DEBUG, "%s: added tag %s", url.first, tag);
			alltags.insert(tag);
		}
	}
}

std::string ttrss_urlreader::get_source()
{
	return "Tiny Tiny RSS";
}
// TODO

} // namespace newsboat

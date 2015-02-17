#include <ttrss_api.h>
#include <logger.h>

namespace newsbeuter {

ttrss_urlreader::ttrss_urlreader(const std::string& url_file, remote_api * a) : file(url_file), api(a) { }

ttrss_urlreader::~ttrss_urlreader() { }

void ttrss_urlreader::write_config() {
	// NOTHING
}

void ttrss_urlreader::reload() {
	urls.clear();
	tags.clear();
	alltags.clear();

	file_urlreader ur(file);
	ur.reload();

	//std::vector<std::string>& file_urls(ur.get_urls());
	auto& file_urls(ur.get_urls());
	for (auto url : file_urls) {
		if (url.substr(0,6) == "query:") {
			urls.push_back(url);
			std::vector<std::string>& file_tags(ur.get_tags(url));
			tags[url] = ur.get_tags(url);
			for (auto tag : file_tags) {
				alltags.insert(tag);
			}
		}
	}

	auto feedurls = api->get_subscribed_urls();

	for (auto url : feedurls) {
		LOG(LOG_DEBUG, "added %s to URL list", url.first.c_str());
		urls.push_back(url.first);
		tags[url.first] = url.second;
		for (auto tag : url.second) {
			LOG(LOG_DEBUG, "%s: added tag %s", url.first.c_str(), tag.c_str());
			alltags.insert(tag);
		}
	}
}

std::string ttrss_urlreader::get_source() {
	return "Tiny Tiny RSS";
}
	// TODO

}

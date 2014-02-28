#include <newsblur_api.h>
#include <logger.h>

namespace newsbeuter {

newsblur_urlreader::newsblur_urlreader(const std::string& url_file, remote_api * a) : file(url_file), api(a) { }

newsblur_urlreader::~newsblur_urlreader() { }

void newsblur_urlreader::write_config() {
	// NOTHING
}

void newsblur_urlreader::reload() {
	urls.clear();
	tags.clear();
	alltags.clear();

	file_urlreader ur(file);
	ur.reload();

	std::vector<std::string>& file_urls(ur.get_urls());
	for(std::vector<std::string>::iterator it=file_urls.begin();it!=file_urls.end();++it) {
		if (it->substr(0,6) == "query:") {
			urls.push_back(*it);
		}
	}

	std::vector<tagged_feedurl> feedurls = api->get_subscribed_urls();

	for (std::vector<tagged_feedurl>::iterator it=feedurls.begin();it!=feedurls.end();++it) {
		LOG(LOG_INFO, "added %s to URL list", it->first.c_str());
		urls.push_back(it->first);
	}
}

std::string newsblur_urlreader::get_source() {
	return "NewsBlur";
}
	// TODO

}

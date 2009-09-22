#include <google_api.h>
#include <logger.h>

namespace newsbeuter {

googlereader_urlreader::googlereader_urlreader(configcontainer * c, remote_api * a) : cfg(c), api(a) { }

googlereader_urlreader::~googlereader_urlreader() { }

void googlereader_urlreader::write_config() {
	// NOTHING
}

void googlereader_urlreader::reload() {
	urls.clear();
	tags.clear();
	alltags.clear();

	std::vector<tagged_feedurl> feedurls = api->get_subscribed_urls();
	for (std::vector<tagged_feedurl>::iterator it=feedurls.begin();it!=feedurls.end();it++) {
		LOG(LOG_DEBUG, "added %s to URL list", it->first.c_str());
		urls.push_back(it->first);
		tags[it->first] = it->second;
		for (std::vector<std::string>::iterator jt=it->second.begin();jt!=it->second.end();jt++) {
			LOG(LOG_DEBUG, "%s: added tag %s", it->first.c_str(), jt->c_str());
			alltags.insert(*jt);
		}
	}
}

std::string googlereader_urlreader::get_source() {
	return "Google Reader";
}

}

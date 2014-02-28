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

	std::vector<std::string>& file_urls(ur.get_urls());
	for(std::vector<std::string>::iterator it=file_urls.begin();it!=file_urls.end();++it) {
		if (it->substr(0,6) == "query:") {
			urls.push_back(*it);
			std::vector<std::string>& file_tags(ur.get_tags(*it));
			tags[*it] = ur.get_tags(*it);
			for (std::vector<std::string>::iterator jt=file_tags.begin();jt!=file_tags.end();++jt) {
				alltags.insert(*jt);
			}
		}
	}

	std::vector<tagged_feedurl> feedurls = api->get_subscribed_urls();

	for (std::vector<tagged_feedurl>::iterator it=feedurls.begin();it!=feedurls.end();++it) {
		LOG(LOG_DEBUG, "added %s to URL list", it->first.c_str());
		urls.push_back(it->first);
		tags[it->first] = it->second;
		for (std::vector<std::string>::iterator jt=it->second.begin();jt!=it->second.end();++jt) {
			LOG(LOG_DEBUG, "%s: added tag %s", it->first.c_str(), jt->c_str());
			alltags.insert(*jt);
		}
	}
}

std::string ttrss_urlreader::get_source() {
	return "Tiny Tiny RSS";
}
	// TODO

}

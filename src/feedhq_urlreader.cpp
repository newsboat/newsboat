#include <feedhq_api.h>
#include <logger.h>

namespace newsbeuter {

feedhq_urlreader::feedhq_urlreader(configcontainer * c, const std::string& url_file, remote_api * a) : cfg(c), file(url_file), api(a) { }

feedhq_urlreader::~feedhq_urlreader() { }

void feedhq_urlreader::write_config() {
	// NOTHING
}

#define BROADCAST_FRIENDS_URL "http://feedhq.org/reader/atom/user/-/state/com.google/broadcast-friends"
#define STARRED_ITEMS_URL "http://feedhq.org/reader/atom/user/-/state/com.google/starred"
#define SHARED_ITEMS_URL "http://feedhq.org/reader/atom/user/-/state/com.google/broadcast"
#define POPULAR_ITEMS_URL "http://feedhq.org/reader/public/atom/pop%2Ftopic%2Ftop%2Flanguage%2Fen"

#define ADD_URL(url,caption) do { \
		tmptags.clear(); \
		urls.push_back((url)); \
		tmptags.push_back((caption)); \
		tags[(url)] = tmptags; } while(0)


void feedhq_urlreader::reload() {
	urls.clear();
	tags.clear();
	alltags.clear();

	if (cfg->get_configvalue_as_bool("feedhq-show-special-feeds")) {
		std::vector<std::string> tmptags;
		ADD_URL(BROADCAST_FRIENDS_URL, std::string("~") + _("People you follow"));
		ADD_URL(STARRED_ITEMS_URL, std::string("~") + _("Starred items"));
		ADD_URL(SHARED_ITEMS_URL, std::string("~") + _("Shared items"));
	}

	file_urlreader ur(file);
	ur.reload();

	std::vector<std::string>& file_urls(ur.get_urls());
	for(std::vector<std::string>::iterator it=file_urls.begin(); it!=file_urls.end(); ++it) {
		if (it->substr(0,6) == "query:") {
			urls.push_back(*it);
			std::vector<std::string>& file_tags(ur.get_tags(*it));
			tags[*it] = ur.get_tags(*it);
			for (std::vector<std::string>::iterator jt=file_tags.begin(); jt!=file_tags.end(); ++jt) {
				alltags.insert(*jt);
			}
		}
	}

	std::vector<tagged_feedurl> feedurls = api->get_subscribed_urls();
	for (std::vector<tagged_feedurl>::iterator it=feedurls.begin(); it!=feedurls.end(); ++it) {
		LOG(LOG_DEBUG, "added %s to URL list", it->first.c_str());
		urls.push_back(it->first);
		tags[it->first] = it->second;
		for (std::vector<std::string>::iterator jt=it->second.begin(); jt!=it->second.end(); ++jt) {
			LOG(LOG_DEBUG, "%s: added tag %s", it->first.c_str(), jt->c_str());
			alltags.insert(*jt);
		}
	}
}

std::string feedhq_urlreader::get_source() {
	return "FeedHQ";
}

}

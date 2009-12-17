#include <google_api.h>
#include <logger.h>

namespace newsbeuter {

googlereader_urlreader::googlereader_urlreader(configcontainer * c, remote_api * a) : cfg(c), api(a) { }

googlereader_urlreader::~googlereader_urlreader() { }

void googlereader_urlreader::write_config() {
	// NOTHING
}

#define BROADCAST_FRIENDS_URL "http://www.google.com/reader/atom/user/-/state/com.google/broadcast-friends"
#define STARRED_ITEMS_URL "http://www.google.com/reader/atom/user/-/state/com.google/starred"
#define SHARED_ITEMS_URL "http://www.google.com/reader/atom/user/-/state/com.google/broadcast"
#define POPULAR_ITEMS_URL "http://www.google.com/reader/public/atom/pop%2Ftopic%2Ftop%2Flanguage%2Fen"

#define ADD_URL(url,caption) do { \
		tmptags.clear(); \
		urls.push_back((url)); \
		tmptags.push_back((caption)); \
		tags[(url)] = tmptags; } while(0)


void googlereader_urlreader::reload() {
	urls.clear();
	tags.clear();
	alltags.clear();

	if (cfg->get_configvalue_as_bool("googlereader-show-special-feeds")) {
		std::vector<std::string> tmptags;
		ADD_URL(BROADCAST_FRIENDS_URL, std::string("~") + _("People you follow"));
		ADD_URL(STARRED_ITEMS_URL, std::string("~") + _("Starred items"));
		ADD_URL(SHARED_ITEMS_URL, std::string("~") + _("Shared items"));
		ADD_URL(POPULAR_ITEMS_URL, std::string("~") + _("Popular items"));
	}

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

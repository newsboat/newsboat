#include <oldreader_api.h>
#include <logger.h>

namespace newsbeuter {

oldreader_urlreader::oldreader_urlreader(configcontainer * c, const std::string& url_file, remote_api * a) : cfg(c), file(url_file), api(a) { }

oldreader_urlreader::~oldreader_urlreader() { }

void oldreader_urlreader::write_config() {
	// NOTHING
}

#define BROADCAST_FRIENDS_URL "http://theoldreader.com/reader/atom/user/-/state/com.google/broadcast-friends"
#define STARRED_ITEMS_URL "http://theoldreader.com/reader/atom/user/-/state/com.google/starred"
#define SHARED_ITEMS_URL "http://theoldreader.com/reader/atom/user/-/state/com.google/broadcast"
#define POPULAR_ITEMS_URL "http://theoldreader.com/reader/public/atom/pop%2Ftopic%2Ftop%2Flanguage%2Fen"

#define ADD_URL(url,caption) do { \
		tmptags.clear(); \
		urls.push_back((url)); \
		tmptags.push_back((caption)); \
		tags[(url)] = tmptags; } while(0)


void oldreader_urlreader::reload() {
	urls.clear();
	tags.clear();
	alltags.clear();

	if (cfg->get_configvalue_as_bool("oldreader-show-special-feeds")) {
		std::vector<std::string> tmptags;
		ADD_URL(BROADCAST_FRIENDS_URL, std::string("~") + _("People you follow"));
		ADD_URL(STARRED_ITEMS_URL, std::string("~") + _("Starred items"));
		ADD_URL(SHARED_ITEMS_URL, std::string("~") + _("Shared items"));
	}

	file_urlreader ur(file);
	ur.reload();

	std::vector<std::string>& file_urls(ur.get_urls());
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

	std::vector<tagged_feedurl> feedurls = api->get_subscribed_urls();
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

std::string oldreader_urlreader::get_source() {
	return "The Old Reader";
}

}

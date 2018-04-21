#include "feedhq_api.h"

#include "logger.h"

namespace newsboat {

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

	for (const auto& url : ur.get_urls()) {
		if (url.substr(0,6) == "query:") {
			urls.push_back(url);

			auto url_tags = ur.get_tags(url);
			tags[url] = url_tags;
			for (const auto& tag : url_tags) {
				alltags.insert(tag);
			}
		}
	}

	std::vector<tagged_feedurl> feedurls = api->get_subscribed_urls();
	for (const auto& tagged : feedurls) {
		std::string url = tagged.first;
		std::vector<std::string> url_tags = tagged.second;

		LOG(level::DEBUG, "added %s to URL list", url);
		urls.push_back(url);
		tags[tagged.first] = url_tags;
		for (const auto& tag : url_tags) {
			LOG(level::DEBUG, "%s: added tag %s", url, tag);
			alltags.insert(tag);
		}
	}
}

std::string feedhq_urlreader::get_source() {
	return "FeedHQ";
}

}

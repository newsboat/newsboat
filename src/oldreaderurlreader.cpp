#include "oldreaderapi.h"

#include "fileurlreader.h"
#include "logger.h"

namespace newsboat {

OldReaderUrlReader::OldReaderUrlReader(ConfigContainer* c,
	const std::string& url_file,
	RemoteApi* a)
	: cfg(c)
	, file(url_file)
	, api(a)
{
}

OldReaderUrlReader::~OldReaderUrlReader() {}

void OldReaderUrlReader::write_config()
{
	// NOTHING
}

#define BROADCAST_FRIENDS_URL                                           \
	"https://theoldreader.com/reader/atom/user/-/state/com.google/" \
	"broadcast-friends"
#define STARRED_ITEMS_URL \
	"https://theoldreader.com/reader/atom/user/-/state/com.google/starred"
#define SHARED_ITEMS_URL                                                \
	"https://theoldreader.com/reader/atom/user/-/state/com.google/" \
	"broadcast"

#define ADD_URL(url, caption)                 \
	do {                                  \
		tmptags.clear();              \
		urls.push_back((url));        \
		tmptags.push_back((caption)); \
		tags[(url)] = tmptags;        \
	} while (0)

void OldReaderUrlReader::reload()
{
	urls.clear();
	tags.clear();
	alltags.clear();

	if (cfg->get_configvalue_as_bool("oldreader-show-special-feeds")) {
		std::vector<std::string> tmptags;
		ADD_URL(BROADCAST_FRIENDS_URL,
			std::string("~") + _("People you follow"));
		ADD_URL(STARRED_ITEMS_URL,
			std::string("~") + _("Starred items"));
		ADD_URL(SHARED_ITEMS_URL, std::string("~") + _("Shared items"));
	}

	FileUrlReader ur(file);
	ur.reload();

	std::vector<std::string>& file_urls(ur.get_urls());
	for (const auto& url : file_urls) {
		if (Utils::is_query_url(url)) {
			urls.push_back(url);
			std::vector<std::string>& file_tags(ur.get_tags(url));
			tags[url] = ur.get_tags(url);
			for (const auto& tag : file_tags) {
				alltags.insert(tag);
			}
		}
	}

	std::vector<tagged_feedurl> feedurls = api->get_subscribed_urls();
	for (const auto& url : feedurls) {
		LOG(Level::DEBUG, "added %s to URL list", url.first);
		urls.push_back(url.first);
		tags[url.first] = url.second;
		for (const auto& tag : url.second) {
			LOG(Level::DEBUG, "%s: added tag %s", url.first, tag);
			alltags.insert(tag);
		}
	}
}

std::string OldReaderUrlReader::get_source()
{
	return "The Old Reader";
}

} // namespace newsboat

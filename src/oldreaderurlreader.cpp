#include "oldreaderurlreader.h"

#include "config.h"
#include "configcontainer.h"
#include "logger.h"
#include "remoteapi.h"
#include "utils.h"

namespace newsboat {

OldReaderUrlReader::OldReaderUrlReader(ConfigContainer* c,
	const Filepath& url_file,
	RemoteApi* a)
	: cfg(c)
	, file(url_file)
	, api(a)
{
}

OldReaderUrlReader::~OldReaderUrlReader() {}

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

std::optional<utils::ReadTextFileError> OldReaderUrlReader::reload()
{
	urls.clear();
	tags.clear();

	if (cfg->get_configvalue_as_bool("oldreader-show-special-feeds")) {
		std::vector<std::string> tmptags;
		ADD_URL(BROADCAST_FRIENDS_URL,
			std::string("~") + _("People you follow"));
		ADD_URL(STARRED_ITEMS_URL,
			std::string("~") + _("Starred items"));
		ADD_URL(SHARED_ITEMS_URL, std::string("~") + _("Shared items"));
	}

	load_query_urls_from_file(file);

	std::vector<TaggedFeedUrl> feedurls = api->get_subscribed_urls();
	for (const auto& url : feedurls) {
		LOG(Level::DEBUG, "added %s to URL list", url.first);
		urls.push_back(url.first);
		tags[url.first] = url.second;
		for (const auto& tag : url.second) {
			LOG(Level::DEBUG, "%s: added tag %s", url.first, tag);
		}
	}

	return {};
}

std::string OldReaderUrlReader::get_source() const
{
	return "The Old Reader";
}

} // namespace newsboat

#include "inoreaderurlreader.h"

#include "config.h"
#include "configcontainer.h"
#include "logger.h"
#include "remoteapi.h"
#include "utils.h"

namespace Newsboat {

InoreaderUrlReader::InoreaderUrlReader(ConfigContainer* c,
	const std::string& url_file,
	RemoteApi* a)
	: cfg(c)
	, file(url_file)
	, api(a)
{
}

InoreaderUrlReader::~InoreaderUrlReader() {}

#define STARRED_ITEMS_URL \
	"http://inoreader.com/reader/atom/user/-/state/com.google/starred"
#define BROADCAST_ITEMS_URL \
	"http://inoreader.com/reader/atom/user/-/state/com.google/broadcast"
#define LIKED_ITEMS_URL \
	"http://inoreader.com/reader/atom/user/-/state/com.google/like"
#define SAVED_WEB_PAGES_ITEMS_URL                                   \
	"http://inoreader.com/reader/atom/user/-/state/com.google/" \
	"saved-web-pages"

#define ADD_URL(url, caption)                 \
	do {                                  \
		tmptags.clear();              \
		urls.push_back((url));        \
		tmptags.push_back((caption)); \
		tags[(url)] = tmptags;        \
	} while (0)

std::optional<utils::ReadTextFileError> InoreaderUrlReader::reload()
{
	urls.clear();
	tags.clear();

	if (cfg->get_configvalue_as_bool("inoreader-show-special-feeds")) {
		std::vector<std::string> tmptags;
		ADD_URL(STARRED_ITEMS_URL,
			std::string("~") + _("Starred items"));
		ADD_URL(BROADCAST_ITEMS_URL,
			std::string("~") + _("Broadcast items"));
		ADD_URL(LIKED_ITEMS_URL, std::string("~") + _("Liked items"));
		ADD_URL(SAVED_WEB_PAGES_ITEMS_URL,
			std::string("~") + _("Saved web pages"));
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

std::string InoreaderUrlReader::get_source() const
{
	return "inoreader";
}

} // namespace Newsboat

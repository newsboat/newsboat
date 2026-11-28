#include "freshrssurlreader.h"

#include "config.h"
#include "configcontainer.h"

namespace newsboat {

FreshRssUrlReader::FreshRssUrlReader(ConfigContainer& c,
	const Filepath& url_file,
	RemoteApi& a)
	: RemoteApiUrlReader("FreshRSS", url_file, a)
	, cfg(c)
{
}

void FreshRssUrlReader::add_special_feeds()
{
	if (cfg.get_configvalue_as_bool("freshrss-show-special-feeds")) {
		const std::string star_url = cfg.get_configvalue("freshrss-url") +
			"/reader/api/0/stream/contents/user/-/state/com.google/starred";
		feed_urls.emplace_back(FeedUrl{star_url, FeedOrigin{}, {std::string("~") + _("Starred items")}});
	}
}

} // namespace newsboat

#include "minifluxurlreader.h"

#include "config.h"
#include "configcontainer.h"

namespace newsboat {

MinifluxUrlReader::MinifluxUrlReader(ConfigContainer& c,
	const Filepath& url_file,
	RemoteApi& a)
	: RemoteApiUrlReader("Miniflux", url_file, a)
	, cfg(c)
{
}

void MinifluxUrlReader::add_special_feeds()
{
	if (cfg.get_configvalue_as_bool("miniflux-show-special-feeds")) {
		feed_urls.emplace_back(FeedUrl{"starred", FeedOrigin{}, {std::string("~") + _("Starred items")}});
	}
}

} // namespace newsboat

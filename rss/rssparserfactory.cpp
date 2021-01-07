#include "rssparserfactory.h"

#include "atomparser.h"
#include "config.h"
#include "exception.h"
#include "rss09xparser.h"
#include "rss10parser.h"
#include "rss20parser.h"

namespace rsspp {

std::shared_ptr<RssParser> RssParserFactory::get_object(
	Feed::Version rss_version,
	xmlDocPtr doc)
{
	switch (rss_version) {
	case Feed::RSS_0_91:
	case Feed::RSS_0_92:
	case Feed::RSS_0_94:
		return std::shared_ptr<RssParser>(new Rss09xParser(doc));
	case Feed::RSS_2_0:
		return std::shared_ptr<RssParser>(new Rss20Parser(doc));
	case Feed::RSS_1_0:
		return std::shared_ptr<RssParser>(new Rss10Parser(doc));
	case Feed::ATOM_0_3:
	case Feed::ATOM_0_3_NONS:
	case Feed::ATOM_1_0:
		return std::shared_ptr<RssParser>(new AtomParser(doc));
	case Feed::UNKNOWN:
	default:
		throw Exception(_("unsupported feed format"));
	}
}

} // namespace rsspp

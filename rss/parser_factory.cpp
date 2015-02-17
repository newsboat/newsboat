/* rsspp - Copyright (C) 2008-2012 Andreas Krennmair <ak@newsbeuter.org>
 * Licensed under the MIT/X Consortium License. See file LICENSE
 * for more information.
 */

#include <config.h>
#include <rsspp_internal.h>

namespace rsspp {

std::shared_ptr<rss_parser> rss_parser_factory::get_object(feed& f, xmlDocPtr doc) {
	switch (f.rss_version) {
		case RSS_0_91:
		case RSS_0_92:
		case RSS_0_94:
			return std::shared_ptr<rss_parser>(new rss_09x_parser(doc));
		case RSS_2_0:
			return std::shared_ptr<rss_parser>(new rss_20_parser(doc));
		case RSS_1_0:
			return std::shared_ptr<rss_parser>(new rss_10_parser(doc));
		case ATOM_0_3:
		case ATOM_0_3_NONS:
		case ATOM_1_0:
			return std::shared_ptr<rss_parser>(new atom_parser(doc));
		case UNKNOWN:
		default:
			throw exception(_("unsupported feed format"));
	}
}

}

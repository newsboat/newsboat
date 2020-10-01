#ifndef NEWSBOAT_RSSPP_MEDIANAMESPACE_H_
#define NEWSBOAT_RSSPP_MEDIANAMESPACE_H_

#include <libxml/tree.h>

#include "item.h"

namespace rsspp {

bool is_media_node(xmlNode* node);
void parse_media_node(xmlNode* node, Item& it);

} // namespace rsspp

#endif // NEWSBOAT_RSSPP_MEDIANAMESPACE_H_

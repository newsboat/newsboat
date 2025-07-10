#ifndef NEWSBOAT_ITEMUTILS_H_
#define NEWSBOAT_ITEMUTILS_H_

#include "cache.h"
#include "rssfeed.h"
#include "rssitem.h"
#include "view.h"

namespace Newsboat {

/// Adds enclosure URL of item to queue.
///
/// Reports outcome on the statusline.
///
/// Returns true if URL was successfully added to queue or was already queued before,
///         false otherwise.
bool enqueue_item_enclosure(RssItem& item, RssFeed& feed,
	View& v, Cache& cache);

} // namespace Newsboat

#endif /* NEWSBOAT_ITEMUTILS_H_ */

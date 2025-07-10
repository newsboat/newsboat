#include "itemutils.h"

#include "controller.h"
#include "strprintf.h"

namespace Newsboat {

bool enqueue_item_enclosure(RssItem& item, RssFeed& feed,
	View& v, Cache& cache)
{
	if (item.enclosure_url().empty()) {
		v.get_statusline().show_error(_("Item has no enclosures."));
		return false;
	} else if (!utils::is_http_url(item.enclosure_url())) {
		v.get_statusline().show_error(strprintf::fmt(
				_("Item's enclosure has non-http link: '%s'"), item.enclosure_url()));
		return false;
	} else {
		const EnqueueResult result = v.get_ctrl().enqueue_url(item, feed);
		cache.update_rssitem_unread_and_enqueued(item, feed.rssurl());
		switch (result.status) {
		case EnqueueStatus::QUEUED_SUCCESSFULLY:
			v.get_statusline().show_message(
				strprintf::fmt(_("Added %s to download queue."),
					item.enclosure_url()));
			return true;
		case EnqueueStatus::URL_QUEUED_ALREADY:
			v.get_statusline().show_message(
				strprintf::fmt(_("%s is already queued."),
					item.enclosure_url()));
			return true; // Not a failure, just an idempotent action
		case EnqueueStatus::OUTPUT_FILENAME_USED_ALREADY:
			v.get_statusline().show_error(
				strprintf::fmt(_("Generated filename (%s) is used already."),
					result.extra_info));
			return false;
		case EnqueueStatus::QUEUE_FILE_OPEN_ERROR:
			v.get_statusline().show_error(
				strprintf::fmt(_("Failed to open queue file: %s."), result.extra_info));
			return false;
		}

		// Not reachable, all switch cases return a result already,
		// and compiler will warn if a switch case is missing.
		return false;
	}
}

} // namespace Newsboat

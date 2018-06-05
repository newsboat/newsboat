#ifndef NEWSBOAT_FEEDHANDLER_H_
#define NEWSBOAT_FEEDHANDLER_H_

#include <memory>
#include <string>
#include <vector>

#include "rss.h"

namespace newsboat {
class FeedHandler {
public:
	FeedHandler();

	void sort_feeds(const std::vector<std::string>& sortmethod_info);

	std::vector<std::shared_ptr<rss_feed>> feeds;

private:
	std::mutex feeds_mutex;
};
} // namespace newsboat

#endif /* NEWSBOAT_FEEDHANDLER_H_ */

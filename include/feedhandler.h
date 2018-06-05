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

	std::vector<std::shared_ptr<rss_feed>> feeds;
};
} // namespace newsboat

#endif /* NEWSBOAT_FEEDHANDLER_H_ */

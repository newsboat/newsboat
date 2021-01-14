#ifndef NEWSBOAT_QUEUEMANAGER_H_
#define NEWSBOAT_QUEUEMANAGER_H_

#include <ctime>
#include <memory>
#include <string>

namespace newsboat {

class ConfigContainer;
class ConfigPaths;
class RssFeed;
class RssItem;

enum class EnqueueResult {
	QUEUED_SUCCESSFULLY,
	URL_QUEUED_ALREADY,
};

class QueueManager {
	ConfigContainer* cfg = nullptr;
	ConfigPaths* paths = nullptr;

public:
	QueueManager(ConfigContainer* cfg, ConfigPaths* paths);

	/// Adds the podcast URL to Podboat's queue file
	EnqueueResult enqueue_url(std::shared_ptr<RssItem> item,
		std::shared_ptr<RssFeed> feed);

	void autoenqueue(std::shared_ptr<RssFeed> feed);

private:
	std::string generate_enqueue_filename(std::shared_ptr<RssItem> item,
		std::shared_ptr<RssFeed> feed);
};

}

#endif /* NEWSBOAT_QUEUEMANAGER_H_ */


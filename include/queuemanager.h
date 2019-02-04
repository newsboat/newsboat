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

class QueueManager {
	ConfigContainer* cfg = nullptr;
	ConfigPaths* paths = nullptr;

public:
	QueueManager(ConfigContainer* cfg, ConfigPaths* paths);

	void enqueue_url(std::shared_ptr<RssItem> item,
		std::shared_ptr<RssFeed> feed);

	void autoenqueue(std::shared_ptr<RssFeed> feed);

private:
	std::string generate_enqueue_filename(std::shared_ptr<RssItem> item,
		std::shared_ptr<RssFeed> feed);
};

}

#endif /* NEWSBOAT_QUEUEMANAGER_H_ */


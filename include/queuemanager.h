#ifndef NEWSBOAT_QUEUEMANAGER_H_
#define NEWSBOAT_QUEUEMANAGER_H_

#include <memory>
#include <string>

namespace newsboat {

class ConfigContainer;
class ConfigPaths;
class RssFeed;

class QueueManager {
	ConfigContainer* cfg = nullptr;
	ConfigPaths* paths = nullptr;

public:
	QueueManager(ConfigContainer* cfg, ConfigPaths* paths);

	void enqueue_url(const std::string& url,
		const std::string& title,
		const time_t pubDate,
		std::shared_ptr<RssFeed> feed);

	void autoenqueue(std::shared_ptr<RssFeed> feed);

private:
	std::string generate_enqueue_filename(const std::string& url,
		const std::string& title,
		const time_t pubDate,
		std::shared_ptr<RssFeed> feed);
};

}

#endif /* NEWSBOAT_QUEUEMANAGER_H_ */


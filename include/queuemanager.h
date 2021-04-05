#ifndef NEWSBOAT_QUEUEMANAGER_H_
#define NEWSBOAT_QUEUEMANAGER_H_

#include <memory>
#include <string>

namespace newsboat {

class ConfigContainer;
class ConfigPaths;
class RssFeed;
class RssItem;

enum class EnqueueStatus {
	QUEUED_SUCCESSFULLY,
	URL_QUEUED_ALREADY, // `extra_info` should specify the concerning URL
	OUTPUT_FILENAME_USED_ALREADY, // `extra_info` should specify the generated filename
	QUEUE_FILE_OPEN_ERROR, // `extra_info` should specify the location of the queue file
};

struct EnqueueResult {
	EnqueueStatus status;
	std::string extra_info;
};

class QueueManager {
	ConfigContainer* cfg = nullptr;
	ConfigPaths* paths = nullptr;

public:
	QueueManager(ConfigContainer* cfg, ConfigPaths* paths);

	/// Adds the podcast URL to Podboat's queue file
	EnqueueResult enqueue_url(std::shared_ptr<RssItem> item,
		std::shared_ptr<RssFeed> feed);

	EnqueueResult autoenqueue(std::shared_ptr<RssFeed> feed);

private:
	std::string generate_enqueue_filename(std::shared_ptr<RssItem> item,
		std::shared_ptr<RssFeed> feed);
};

}

#endif /* NEWSBOAT_QUEUEMANAGER_H_ */


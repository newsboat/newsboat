#ifndef NEWSBOAT_QUEUEMANAGER_H_
#define NEWSBOAT_QUEUEMANAGER_H_

#include <string>

namespace Newsboat {

class ConfigContainer;
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
	std::string queue_file;

public:
	/// Construct `QueueManager` instance out of a config container and a path
	/// to the queue file.
	QueueManager(ConfigContainer* cfg, std::string queue_file);

	/// Adds the podcast URL to Podboat's queue file
	EnqueueResult enqueue_url(RssItem& item, RssFeed& feed);

	/// Add all HTTP and HTTPS enclosures to the queue file
	EnqueueResult autoenqueue(RssFeed& feed);

private:
	std::string generate_enqueue_filename(RssItem& item, RssFeed& feed);
};

}

#endif /* NEWSBOAT_QUEUEMANAGER_H_ */


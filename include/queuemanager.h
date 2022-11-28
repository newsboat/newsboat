#ifndef NEWSBOAT_QUEUEMANAGER_H_
#define NEWSBOAT_QUEUEMANAGER_H_

#include <memory>
#include <string>
#include <mpd/connection.h>
#include <mpd/queue.h>
#include <mpd/search.h>
#include <mpd/song.h>
#include <mpd/error.h>

typedef enum mpd_error (*mpd_connection_get_error_t)(const struct mpd_connection *connection);
typedef const char* (*mpd_connection_get_error_message_t)(const struct mpd_connection *connection);
typedef void (*mpd_connection_free_t)(struct mpd_connection* connection);
typedef struct mpd_connection* (*mpd_connection_new_t)(const char* host, unsigned port, unsigned timeout_ms);
typedef bool (*mpd_search_queue_songs_t)(struct mpd_connection* connection, bool exact);
typedef bool (*mpd_search_add_uri_constraint_t)(struct mpd_connection* connection,
					       enum mpd_operator oper, const char* value);
typedef bool (*mpd_search_commit_t)(struct mpd_connection* connection);
typedef struct mpd_song* (*mpd_recv_song_t)(struct mpd_connection* connection);
typedef bool (*mpd_run_add_t)(struct mpd_connection* connection,
			     const char* uri);

namespace newsboat {

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

	bool init();
	void deinit();
	/// Adds the podcast URL to Podboat's queue file
	EnqueueResult enqueue_url(std::shared_ptr<RssItem> item,
		std::shared_ptr<RssFeed> feed);

	/// Add all HTTP and HTTPS enclosures to the queue file
	EnqueueResult autoenqueue(std::shared_ptr<RssFeed> feed);

private:
	std::string generate_enqueue_filename(std::shared_ptr<RssItem> item,
		std::shared_ptr<RssFeed> feed);
	void* mpd_handle;
	struct mpd_connection* mpd_connection;
	mpd_connection_get_error_t mpd_connection_get_error_;
	mpd_connection_get_error_message_t mpd_connection_get_error_message_;
	mpd_search_queue_songs_t mpd_search_queue_songs_;
	mpd_search_add_uri_constraint_t mpd_search_add_uri_constraint_;
	mpd_search_commit_t mpd_search_commit_;
	mpd_recv_song_t mpd_recv_song_;
	mpd_run_add_t mpd_run_add_;
	mpd_connection_free_t mpd_connection_free_;
	mpd_connection_new_t mpd_connection_new_;
};

}

#endif /* NEWSBOAT_QUEUEMANAGER_H_ */


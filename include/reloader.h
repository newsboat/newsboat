#ifndef NEWSBOAT_RELOADER_H_
#define NEWSBOAT_RELOADER_H_

#include <atomic>
#include <functional>
#include <mutex>
#include <vector>

#include "configcontainer.h"

namespace newsboat {

class Cache;
class Controller;
class CurlHandle;

/// \brief Updates feeds (fetches, parses, puts results into Controller).
class Reloader {
public:
	Reloader(Controller* c, Cache* cc, ConfigContainer* cfg);

	/// \brief Creates detached thread that runs periodic updates.
	void spawn_reloadthread();

	/// \brief Starts a thread that will reload feeds with specified
	/// indexes.
	///
	/// If \a indexes is empty, all feeds will be reloaded.
	void start_reload_all_thread(const std::vector<int>& indexes = {});

	/// \brief Reloads given feed.
	///
	/// Reloads the feed at position \a pos in the feeds list (as kept by
	/// feedscontainer). Only updates status bar if \a unattended is false.
	void reload(unsigned int pos, bool unattended = false)
	{
		reload(pos, false, unattended);
	}

	/// \brief Reloads all feeds, spawning threads as necessary.
	///
	/// Only updates status bar if \a unattended is false. The number of
	/// threads spawned is controlled by the user via reload-threads
	/// setting.
	void reload_all(bool unattended = false);

	/// \brief Reloads all feeds with given indexes in feedlist.
	///
	/// Only updates status bar if \a unattended is false.
	void reload_indexes(const std::vector<int>& indexes,
		bool unattended = false);

	/// \brief Reloads feeds occupying positions from \a start to \a end in
	/// feedlist.
	///
	/// Only updates status bar if \a unattended is false.
	void reload_range(unsigned int start,
		unsigned int end,
		bool unattended = false);

private:
	/// \brief Reloads given feed.
	///
	/// Reloads the feed at position \a pos in the feeds list (as kept by
	/// feedscontainer). \a show_progress specifies if a progress indicator
	/// (`[<progress>/<total_feeds>]`) should be included when updating the status
	/// message (at the bottom of the screen). Status messages are only shown
	/// if \a unattended is false. All network requests are made through
	/// \a easyhandle. If the handle is not provided, this method creates
	/// a temporary handle which is destroyed before returning from it.
	void reload(unsigned int pos,
		bool show_progress,
		bool unattended);

	void reload(unsigned int pos,
		CurlHandle& easyhandle,
		bool show_progress,
		bool unattended);

	/// \brief Notify in various ways that there are new unread feeds or
	/// articles.
	///
	/// The type of notification is based on "notify-screen", "notify-xterm",
	/// "notify-beep" and "notify-program" settings chosen in Newsboat's config
	/// file.
	///
	/// If "notify-screen", "notify-xterm" or "notify-program" is chosen, the
	/// notification will contain \a msg passed.
	void notify(const std::string& msg);

	void notify_reload_finished(unsigned int unread_feeds_before,
		unsigned int unread_articles_before);

	void unlock_reload_mutex()
	{
		reload_mutex.unlock();
	}
	bool trylock_reload_mutex();

	void partition_reload_to_threads(
		std::function<void(unsigned int start, unsigned int end)> handle_range,
		unsigned int num_feeds);

	Controller* ctrl;
	Cache* rsscache;
	ConfigContainer* cfg;
	std::mutex reload_mutex;
	std::atomic<unsigned int> reload_progress;
	unsigned int reload_progress_max;
};

} // namespace newsboat

#endif /* NEWSBOAT_RELOADER_H_ */

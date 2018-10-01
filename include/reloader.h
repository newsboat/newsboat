#ifndef NEWSBOAT_RELOADER_H_
#define NEWSBOAT_RELOADER_H_

#include <mutex>
#include <vector>

namespace newsboat {

class Controller;
class CurlHandle;

/// \brief Updates feeds (fetches, parses, puts results into Controller).
class Reloader {
	Controller* ctrl;
	std::mutex reload_mutex;

	std::string prepare_message(unsigned int pos, unsigned int max);

public:
	Reloader(Controller* c);

	/// \brief Creates detached thread that runs periodic updates.
	void spawn_reloadthread();

	/// \brief Starts a thread that will reload feeds with specified
	/// indexes.
	///
	/// If \a indexes is nullptr, all feeds will be reloaded.
	void start_reload_all_thread(std::vector<int>* indexes = nullptr);

	void unlock_reload_mutex()
	{
		reload_mutex.unlock();
	}
	bool trylock_reload_mutex();

	/// \brief Reloads given feed.
	///
	/// Reloads the feed at position \a pos in the feeds list (as kept by
	/// feedscontainer). \a max is a total amount of feeds (used when
	/// preparing messages to the user). Only updates status (at the bottom
	/// of the screen) if \a unattended is false. All network requests are
	/// made through \a easyhandle, unless it's nullptr, in which case
	/// method creates a temporary handle that is destroyed when method
	/// completes.
	// TODO: check that the value passed via "max" is always obtained from
	// feedcontainer, then move that request into the method and drop the
	// parameter.
	void reload(unsigned int pos,
		unsigned int max = 0,
		bool unattended = false,
		CurlHandle* easyhandle = nullptr);

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
		unsigned int size,
		bool unattended = false);

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
};

} // namespace newsboat

#endif /* NEWSBOAT_RELOADER_H_ */

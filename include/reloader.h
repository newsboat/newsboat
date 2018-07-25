#ifndef NEWSBOAT_RELOADER_H_
#define NEWSBOAT_RELOADER_H_

#include <mutex>
#include <vector>

namespace newsboat {

class controller;

/// \brief Updates feeds (fetches, parses, puts results into controller).
class Reloader {
	controller* ctrl;
	std::mutex reload_mutex;

public:
	Reloader(controller* c);

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
};

} // namespace newsboat

#endif /* NEWSBOAT_RELOADER_H_ */

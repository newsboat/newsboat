#ifndef NEWSBOAT_RELOADER_H_
#define NEWSBOAT_RELOADER_H_

namespace newsboat {

class controller;

/// \brief Updates feeds (fetches, parses, puts results into controller).
class Reloader {
	controller* ctrl;

public:
	Reloader(controller* c);

	/// \brief Creates detached thread that runs periodic updates.
	void spawn_reloadthread();
};

} // namespace newsboat

#endif /* NEWSBOAT_RELOADER_H_ */

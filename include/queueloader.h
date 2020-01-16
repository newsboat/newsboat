#ifndef PODBOAT_QUEUELOADER_H_
#define PODBOAT_QUEUELOADER_H_

#include <functional>
#include <vector>

#include "configcontainer.h"
#include "download.h"

namespace podboat {

class QueueLoader {
public:
	QueueLoader(const std::string& file, newsboat::ConfigContainer& cfg,
		std::function<void()> cb_require_view_update);
	void reload(std::vector<Download>& downloads,
		bool remove_unplayed = false);

private:
	std::string get_filename(const std::string& str);
	std::string queuefile;
	newsboat::ConfigContainer& cfg;
	std::function<void()> cb_require_view_update;
};

} // namespace podboat

#endif /* PODBOAT_QUEUELOADER_H_ */

#ifndef PODBOAT_QUEUELOADER_H_
#define PODBOAT_QUEUELOADER_H_

#include <vector>

#include "download.h"
#include "pbcontroller.h"

namespace podboat {

class QueueLoader {
public:
	QueueLoader(const std::string& file, PbController* c = 0);
	void reload(std::vector<Download>& downloads,
		bool remove_unplayed = false);

private:
	std::string get_filename(const std::string& str);
	std::string queuefile;
	PbController* ctrl;
};

} // namespace podboat

#endif /* PODBOAT_QUEUELOADER_H_ */

#ifndef PODBOAT_QUEUELOADER_H_
#define PODBOAT_QUEUELOADER_H_

#include <vector>

#include "download.h"
#include "pb_controller.h"

namespace podboat {

class queueloader {
public:
	queueloader(const std::string& file, pb_controller* c = 0);
	void
	reload(std::vector<download>& downloads, bool remove_unplayed = false);

private:
	std::string get_filename(const std::string& str);
	std::string queuefile;
	pb_controller* ctrl;
};

} // namespace podboat

#endif /* PODBOAT_QUEUELOADER_H_ */

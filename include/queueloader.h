#ifndef PODBEUTER_QUEUELOADER__H
#define PODBEUTER_QUEUELOADER__H

#include <vector>
#include <download.h>
#include <pb_controller.h>

namespace podbeuter {

	class queueloader {
		public:
			queueloader(const std::string& file, pb_controller * c = 0);
			void load(std::vector<download>& downloads);
		private:
			std::string queuefile;
			pb_controller * ctrl;
	};

}

#endif

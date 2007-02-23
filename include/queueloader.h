#ifndef PODBEUTER_QUEUELOADER__H
#define PODBEUTER_QUEUELOADER__H

#include <vector>
#include <download.h>

namespace podbeuter {

	class queueloader {
		public:
			queueloader(const std::string& file);
			void load(std::vector<download>& downloads);
		private:
			std::string queuefile;
	};

}

#endif

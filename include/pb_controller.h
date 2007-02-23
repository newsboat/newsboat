#ifndef PODBEUTER_CONTROLLER__H
#define PODBEUTER_CONTROLLER__H

#include <string>

#include <configcontainer.h>

namespace podbeuter {

	class pb_view;

	class pb_controller {
		public:
			pb_controller();
			~pb_controller();
			inline void set_view(pb_view * vv) { v = vv; }
			void run(int argc, char * argv[] = 0);
		private:
			pb_view * v;
			std::string config_file;
			newsbeuter::configcontainer * cfg;
	};

}

#endif

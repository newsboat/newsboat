#ifndef NOOS_CONTROLLER__H
#define NOOS_CONTROLLER__H

#include <configreader.h>

namespace noos {

	class view;

	class controller {
		public:
			controller();
			void set_view(view * vv);
			void run();
			void open_feed(unsigned int pos);
		private:
			view * v;
			configreader cfg;
	};

}


#endif

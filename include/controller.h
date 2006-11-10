#ifndef NOOS_CONTROLLER__H
#define NOOS_CONTROLLER__H

namespace noos {

	class view;

	class controller {
		public:
			controller();
			void set_view(view * vv);
			void run();
		private:
			view * v;
	};

}


#endif

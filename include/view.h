#ifndef NOOS_VIEW__H
#define NOOS_VIEW__H

#include <controller.h>

extern "C" {
#include <stfl.h>
}

namespace noos {

	class view {
		public:
			view(controller * );
			~view();
			void run_feedlist();
			void run_itemlist();
			void run_itemview();
		private:
			controller * ctrl;
			stfl_form * feedlist_form;
			stfl_form * itemlist_form;
			stfl_form * itemview_form;
	};

}

#endif

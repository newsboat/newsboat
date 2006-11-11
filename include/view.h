#ifndef NOOS_VIEW__H
#define NOOS_VIEW__H

#include <controller.h>
#include <vector>
#include <string>
#include <rss.h>

extern "C" {
#include <stfl.h>
}

namespace noos {

	class view {
		public:
			view(controller * );
			~view();
			void run_feedlist();
			void run_itemlist(rss_feed& feed);
			void run_itemview();
			void set_feedlist(const std::vector<std::string>& feeds);
			void feedlist_error(char * msg);
			void feedlist_status(char * msg);
		private:
			controller * ctrl;
			stfl_form * feedlist_form;
			stfl_form * itemlist_form;
			stfl_form * itemview_form;
	};

}

#endif

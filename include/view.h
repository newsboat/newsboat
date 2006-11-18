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
			void run_itemview(rss_item& item);
			void set_feedlist(std::vector<rss_feed>& feeds);
			void feedlist_error(const char * msg);
			void itemlist_error(const char * msg);
			void feedlist_status(const char * msg);
			void itemlist_status(const char * msg);
		private:
			void jump_to_next_unread_item(std::vector<rss_item>& items);
			void mark_all_read(std::vector<rss_item>& items);
			void open_in_browser(const std::string& url);

			controller * ctrl;
			stfl_form * feedlist_form;
			stfl_form * itemlist_form;
			stfl_form * itemview_form;
	};

}

#endif

#ifndef NEWSBEUTER_VIEW__H
#define NEWSBEUTER_VIEW__H

#include <controller.h>
#include <configcontainer.h>
#include <vector>
#include <list>
#include <string>
#include <rss.h>
#include <keymap.h>
#include <mutex.h>

#include <stflpp.h>

/*
extern "C" {
#include <stfl.h>
}
*/

namespace newsbeuter {

	class view {
		public:
			view(controller * );
			~view();
			void run_feedlist(const std::vector<std::string> & tags);
			bool run_itemlist(unsigned int pos, bool auto_open);
			bool run_itemview(const rss_feed& feed, rss_item& item);
			void run_help();
			void run_search(const std::string& feed = "");
			std::string select_tag(const std::vector<std::string>& tags);
			void set_feedlist(std::vector<rss_feed>& feeds);
			void set_keymap(keymap * k);
			void set_config_container(configcontainer * cfgcontainer);
			void show_error(const char * msg);
			void set_status(const char * msg);
		private:
			bool jump_to_next_unread_item(std::vector<rss_item>& items);
			bool jump_to_next_unread_feed();
			void mark_all_read(std::vector<rss_item>& items);
			void open_in_browser(const std::string& url);
			
			void set_itemview_keymap_hint();
			void set_itemlist_keymap_hint();
			void set_feedlist_keymap_hint();
			void set_help_keymap_hint();
			void set_filebrowser_keymap_hint();
			void set_urlview_keymap_hint();
			void set_selecttag_keymap_hint();
			
			void set_itemlist_head(const std::string& s, unsigned int unread, unsigned int total, const std::string &url);
			void set_itemview_head(const std::string& s);
			
			void write_item(const rss_item& item, const std::string& filename);
			
			enum filebrowser_type { FBT_OPEN, FBT_SAVE };
			
			std::string get_rwx(unsigned short val);
			std::string add_file(std::string filename);
			std::string fancy_quote(const std::string& s);
			std::string fancy_unquote(const std::string& s);
			std::string filebrowser(filebrowser_type type, const std::string& default_filename = "", std::string dir = "");
			std::string get_filename_suggestion(const std::string& s);
			void render_source(std::vector<std::string>& lines, std::string desc, unsigned int width);

			void run_urlview(std::vector<std::string>& links);
			
			struct keymap_hint_entry {
				operation op; 
				char * text;
			};

			std::string prepare_keymaphint(keymap_hint_entry * hints);

			controller * ctrl;

			configcontainer * cfg;
			keymap * keys;
			mutex * mtx;

			friend class colormanager;

			stfl::form feedlist_form;
			stfl::form itemlist_form;
			stfl::form itemview_form;
			stfl::form help_form;
			stfl::form filebrowser_form;
			stfl::form urlview_form;
			stfl::form selecttag_form;
			
			std::list<stfl::form *> view_stack;
			
			std::vector<std::pair<rss_feed *, unsigned int> > visible_feeds;

			std::string tag;

			unsigned int feeds_shown;
	};

}

#endif

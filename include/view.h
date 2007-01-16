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

extern "C" {
#include <stfl.h>
}

namespace newsbeuter {

	class view {
		public:
			view(controller * );
			~view();
			void run_feedlist();
			void run_itemlist(unsigned int pos);
			bool run_itemview(rss_item& item);
			void run_help();
			void set_feedlist(std::vector<rss_feed>& feeds);
			void set_keymap(keymap * k);
			void set_config_container(configcontainer * cfgcontainer);
			void show_error(const char * msg);
			void set_status(const char * msg);
		private:
			bool jump_to_next_unread_item(std::vector<rss_item>& items);
			void jump_to_next_unread_feed();
			void mark_all_read(std::vector<rss_item>& items);
			void open_in_browser(const std::string& url);
			
			void set_itemview_keymap_hint();
			void set_itemlist_keymap_hint();
			void set_feedlist_keymap_hint();
			void set_help_keymap_hint();
			void set_filebrowser_keymap_hint();
			
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
			
			struct keymap_hint_entry {
				operation op; 
				char * text;
			};

			std::string prepare_keymaphint(keymap_hint_entry * hints);

			controller * ctrl;
			stfl_form * feedlist_form;
			stfl_form * itemlist_form;
			stfl_form * itemview_form;
			stfl_form * help_form;
			stfl_form * filebrowser_form;
			
			std::list<stfl_form *> view_stack;
			
			configcontainer * cfg;
			keymap * keys;
			mutex * mtx;
			unsigned int feeds_shown;
	};

}

#endif

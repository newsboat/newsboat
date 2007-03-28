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
#include <htmlrenderer.h>

#include <stflpp.h>
#include <filebrowser_formaction.h>

/*
extern "C" {
#include <stfl.h>
}
*/

namespace newsbeuter {

	class formaction;
	class feedlist_formaction;
	class itemlist_formaction;
	class itemview_formaction;
	class help_formaction;
	class urlview_formaction;

	class view {
		public:
			view(controller * );
			~view();
			void run();
			std::string run_modal(formaction * f, const std::string& value);

			/*
			void run_feedlist(const std::vector<std::string> & tags);
			bool run_itemlist(unsigned int pos, bool auto_open);
			bool run_itemview(rss_feed& feed, std::string guid);
			void run_help();
			void run_search(const std::string& feed = "");
			std::string select_tag(const std::vector<std::string>& tags);
			*/
			void set_feedlist(std::vector<rss_feed>& feeds);
			void set_keymap(keymap * k);
			void set_config_container(configcontainer * cfgcontainer);
			void show_error(const char * msg);
			void set_status(const char * msg);
			inline controller * get_ctrl() { return ctrl; }
			inline configcontainer * get_cfg() { return cfg; }
			inline keymap * get_keys() { return keys; }
			void set_tags(const std::vector<std::string>& t);
			void pop_current_formaction();


			void write_item(const rss_item& item, const std::string& filename);

			void push_itemlist(unsigned int pos);
			void push_itemview(rss_feed * f, const std::string& guid);
			void push_help();
			void view::push_urlview(const std::vector<linkpair>& links);

			std::string run_filebrowser(filebrowser_type type, const std::string& default_filename = "", const std::string& dir = "");

			void render_source(std::vector<std::string>& lines, std::string desc, unsigned int width);
			void open_in_browser(const std::string& url);

			std::string fancy_quote(const std::string& s);
			std::string fancy_unquote(const std::string& s);
			std::string add_file(std::string filename);
			std::string get_filename_suggestion(const std::string& s);

		protected:
			bool jump_to_next_unread_item(std::vector<rss_item>& items, bool begin_with_next);
			bool jump_to_next_unread_feed(bool begin_with_next);
			
			void set_itemview_keymap_hint();
			void set_itemlist_keymap_hint();
			void set_feedlist_keymap_hint();
			void set_help_keymap_hint();
			void set_filebrowser_keymap_hint();
			void set_urlview_keymap_hint();
			void set_selecttag_keymap_hint();
			void set_search_keymap_hint();
			
			void set_itemview_head(const std::string& s);
			
			std::string get_rwx(unsigned short val);
			// std::string filebrowser(filebrowser_type type, const std::string& default_filename = "", std::string dir = "");

			
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

			/*
			stfl::form feedlist_form;
			stfl::form itemlist_form;
			stfl::form itemview_form;
			stfl::form help_form;
			stfl::form filebrowser_form;
			stfl::form urlview_form;
			stfl::form selecttag_form;
			stfl::form search_form;
			*/
			feedlist_formaction * feedlist;
			itemlist_formaction * itemlist;
			itemview_formaction * itemview;
			help_formaction * helpview;
			filebrowser_formaction * filebrowser;
			urlview_formaction * urlview;
			
			std::list<formaction *> formaction_stack;
			
			unsigned int feeds_shown;
	};

}

#endif

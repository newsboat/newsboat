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
#include <colormanager.h>

#include <stflpp.h>
#include <filebrowser_formaction.h>

namespace newsbeuter {

	class formaction;
	class feedlist_formaction;
	class itemlist_formaction;
	class itemview_formaction;
	class help_formaction;
	class urlview_formaction;
	class select_formaction;

	class view {
		public:
			view(controller * );
			~view();
			void run();
			std::string run_modal(formaction * f, const std::string& value = "");

			void set_feedlist(std::vector<rss_feed>& feeds);
			void set_keymap(keymap * k);
			void set_config_container(configcontainer * cfgcontainer);
			void show_error(const char * msg);
			void set_status(const char * msg);
			void set_status_unlocked(const char * msg);
			inline controller * get_ctrl() { return ctrl; }
			inline configcontainer * get_cfg() { return cfg; }
			inline keymap * get_keys() { return keys; }
			void set_tags(const std::vector<std::string>& t);
			void pop_current_formaction();
			inline unsigned int formaction_stack_size() { return formaction_stack.size(); }
			char confirm(const std::string& prompt, const std::string& charset);

			void write_item(const rss_item& item, const std::string& filename);

			void push_itemlist(unsigned int pos);
			void push_itemlist(rss_feed * feed);
			void push_itemview(rss_feed * f, const std::string& guid);
			void push_help();
			void push_urlview(const std::vector<linkpair>& links);
			void push_searchresult(rss_feed * feed);

			std::string run_filebrowser(filebrowser_type type, const std::string& default_filename = "", const std::string& dir = "");
			std::string select_tag(const std::vector<std::string>& tags);
			std::string select_filter(const std::vector<filter_name_expr_pair>& filters);

			void open_in_browser(const std::string& url);

			std::string get_filename_suggestion(const std::string& s);

			bool get_next_unread();
			bool get_previous_unread();

			bool get_next_unread_feed();
			bool get_prev_unread_feed();

			void set_colors(const colormanager& colorman);

			void notify_itemlist_change(rss_feed& feed);

		protected:
			/*
			bool jump_to_next_unread_item(std::vector<rss_item>& items, bool begin_with_next);
			bool jump_to_next_unread_feed(bool begin_with_next);
			bool jump_to_previous_unread_item(std::vector<rss_item>& items, bool begin_with_prev);
			bool jump_to_previous_unread_feed(bool begin_with_prev);
			*/

			void set_bindings();

			controller * ctrl;

			configcontainer * cfg;
			keymap * keys;
			mutex * mtx;

			friend class colormanager;

			feedlist_formaction * feedlist;
			itemlist_formaction * itemlist;
			itemview_formaction * itemview;
			help_formaction * helpview;
			filebrowser_formaction * filebrowser;
			urlview_formaction * urlview;
			select_formaction * selecttag;
			itemlist_formaction * searchresult;

			std::list<formaction *> formaction_stack;

			unsigned int feeds_shown;
	};

}

#endif

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
#include <regexmanager.h>

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
			view(controller *);
			~view();
			void run();
			std::string run_modal(formaction * f, const std::string& value = "");

			std::string id() const;

			void set_feedlist(std::vector<std::tr1::shared_ptr<rss_feed> >& feeds);
			void update_visible_feeds(std::vector<std::tr1::shared_ptr<rss_feed> >& feeds);
			void set_keymap(keymap * k);
			void set_config_container(configcontainer * cfgcontainer);
			void show_error(const std::string& msg);
			void set_status(const std::string& msg);
			void set_status_unlocked(const std::string& msg);
			inline controller * get_ctrl() { return ctrl; }
			inline configcontainer * get_cfg() { return cfg; }
			inline keymap * get_keys() { return keys; }
			void set_tags(const std::vector<std::string>& t);
			void pop_current_formaction();
			inline unsigned int formaction_stack_size() { return formaction_stack.size(); }
			char confirm(const std::string& prompt, const std::string& charset);

			void push_itemlist(unsigned int pos);
			void push_itemlist(std::tr1::shared_ptr<rss_feed> feed);
			void push_itemview(std::tr1::shared_ptr<rss_feed> f, const std::string& guid);
			void push_help();
			void push_urlview(const std::vector<linkpair>& links);
			void push_searchresult(std::tr1::shared_ptr<rss_feed> feed);

			std::string run_filebrowser(filebrowser_type type, const std::string& default_filename = "", const std::string& dir = "");
			std::string select_tag(const std::vector<std::string>& tags);
			std::string select_filter(const std::vector<filter_name_expr_pair>& filters);

			void open_in_browser(const std::string& url);

			std::string get_filename_suggestion(const std::string& s);

			bool get_next_unread();
			bool get_previous_unread();

			bool get_next_unread_feed();
			bool get_prev_unread_feed();

			void set_colors(std::map<std::string,std::string>& fg_colors, std::map<std::string,std::string>& bg_colors, std::map<std::string,std::vector<std::string> >& attributes);

			void notify_itemlist_change(std::tr1::shared_ptr<rss_feed>& feed);

			std::string ask_user(const std::string& prompt);

			void feedlist_mark_pos_if_visible(unsigned int pos);

			void set_regexmanager(regexmanager * r);

		protected:

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

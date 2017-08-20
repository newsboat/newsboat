#ifndef NEWSBEUTER_VIEW__H
#define NEWSBEUTER_VIEW__H

#include <controller.h>
#include <configcontainer.h>
#include <vector>
#include <list>
#include <string>
#include <rss.h>
#include <keymap.h>
#include <mutex>
#include <htmlrenderer.h>
#include <colormanager.h>
#include <regexmanager.h>

#include <stflpp.h>
#include <filebrowser_formaction.h>

namespace newsbeuter {

class formaction;
class itemlist_formaction;
class itemview_formaction;

class view {
	public:
		explicit view(controller *);
		~view();
		void run();
		std::string run_modal(std::shared_ptr<formaction> f, const std::string& value = "");

		void set_feedlist(std::vector<std::shared_ptr<rss_feed>> feeds);
		void update_visible_feeds(std::vector<std::shared_ptr<rss_feed>> feeds);
		void set_keymap(keymap * k);
		void set_config_container(configcontainer * cfgcontainer);
		void show_error(const std::string& msg);
		void set_status(const std::string& msg);
		void set_status_unlocked(const std::string& msg);
		inline controller * get_ctrl() {
			return ctrl;
		}
		inline configcontainer * get_cfg() {
			return cfg;
		}
		inline keymap * get_keys() {
			return keys;
		}
		void set_tags(const std::vector<std::string>& t);
		void push_empty_formaction();
		void pop_current_formaction();
		void remove_formaction(unsigned int pos);
		void set_current_formaction(unsigned int pos);
		inline unsigned int formaction_stack_size() {
			return formaction_stack.size();
		}
		char confirm(const std::string& prompt, const std::string& charset);

		void push_itemlist(unsigned int pos);
		void push_itemlist(std::shared_ptr<rss_feed> feed);
		void push_itemview(std::shared_ptr<rss_feed> f, const std::string& guid, const std::string& searchphrase = "");
		void push_help();
		void push_urlview(const std::vector<linkpair>& links, std::shared_ptr<rss_feed>& feed);
		void push_searchresult(std::shared_ptr<rss_feed> feed, const std::string& phrase = "");
		void view_dialogs();

		std::string run_filebrowser(const std::string& default_filename = "", const std::string& dir = "");
		std::string select_tag();
		std::string select_filter(const std::vector<filter_name_expr_pair>& filters);

		bool open_in_browser(const std::string& url);
		void open_in_pager(const std::string& filename);

		std::string get_filename_suggestion(const std::string& s);

		bool get_next_unread(itemlist_formaction * itemlist, itemview_formaction * itemview = nullptr);
		bool get_previous_unread(itemlist_formaction * itemlist, itemview_formaction * itemview = nullptr);
		bool get_next(itemlist_formaction * itemlist, itemview_formaction * itemview = nullptr);
		bool get_previous(itemlist_formaction * itemlist, itemview_formaction * itemview = nullptr);
		bool get_random_unread(itemlist_formaction * itemlist, itemview_formaction * itemview = nullptr);

		bool get_next_unread_feed(itemlist_formaction * itemlist);
		bool get_prev_unread_feed(itemlist_formaction * itemlist);
		bool get_next_feed(itemlist_formaction * itemlist);
		bool get_prev_feed(itemlist_formaction * itemlist);

		void prepare_query_feed(std::shared_ptr<rss_feed> feed);

		void force_redraw();

		void set_colors(std::map<std::string,std::string>& fg_colors, std::map<std::string,std::string>& bg_colors, std::map<std::string,std::vector<std::string>>& attributes);

		void notify_itemlist_change(std::shared_ptr<rss_feed> feed);

		void feedlist_mark_pos_if_visible(unsigned int pos);

		void set_regexmanager(regexmanager * r);

		std::vector<std::pair<unsigned int, std::string>> get_formaction_names();

		std::shared_ptr<formaction> get_current_formaction();

		std::shared_ptr<formaction> get_formaction(unsigned int idx) const {
			return formaction_stack[idx];
		}

		void goto_next_dialog();
		void goto_prev_dialog();

		void apply_colors_to_all_formactions();

		void update_bindings();

		void inside_qna(bool f);
		void inside_cmdline(bool f);

		void dump_current_form();

	protected:
		void set_bindings(std::shared_ptr<formaction> fa);
		void apply_colors(std::shared_ptr<formaction> fa);

		void handle_cmdline_completion(std::shared_ptr<formaction> fa);
		void clear_line(std::shared_ptr<formaction> fa);
		void clear_eol(std::shared_ptr<formaction> fa);
		void cancel_input(std::shared_ptr<formaction> fa);
		void delete_word(std::shared_ptr<formaction> fa);

		controller * ctrl;

		configcontainer * cfg;
		keymap * keys;
		std::mutex mtx;

		friend class colormanager;

		std::vector<std::shared_ptr<formaction>> formaction_stack;
		unsigned int current_formaction;

		std::vector<std::string> tags;
		unsigned int feeds_shown;

		regexmanager * rxman;

		std::map<std::string,std::string> fg_colors;
		std::map<std::string,std::string> bg_colors;
		std::map<std::string,std::vector<std::string>> attributes;

		bool is_inside_qna;
		bool is_inside_cmdline;

		std::string last_fragment;
		unsigned int tab_count;
		std::vector<std::string> suggestions;
};

}

#endif

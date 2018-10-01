#ifndef NEWSBOAT_VIEW_H_
#define NEWSBOAT_VIEW_H_

#include <list>
#include <mutex>
#include <string>
#include <vector>

#include "colormanager.h"
#include "configcontainer.h"
#include "controller.h"
#include "filebrowserformaction.h"
#include "htmlrenderer.h"
#include "keymap.h"
#include "regexmanager.h"
#include "rss.h"
#include "stflpp.h"

namespace newsboat {

class Formaction;
class ItemListFormaction;
class ItemViewFormaction;

class View {
public:
	explicit View(Controller*);
	~View();
	int run();
	std::string run_modal(std::shared_ptr<Formaction> f,
		const std::string& value = "");

	void set_feedlist(std::vector<std::shared_ptr<RssFeed>> feeds);
	void update_visible_feeds(std::vector<std::shared_ptr<RssFeed>> feeds);
	void set_keymap(Keymap* k);
	void set_config_container(ConfigContainer* cfgcontainer);
	void show_error(const std::string& msg);
	void set_status(const std::string& msg);
	void set_status_unlocked(const std::string& msg);
	Controller* get_ctrl()
	{
		return ctrl;
	}
	ConfigContainer* get_cfg()
	{
		return cfg;
	}
	Keymap* get_keys()
	{
		return keys;
	}
	void set_tags(const std::vector<std::string>& t);
	void push_empty_formaction();
	void pop_current_formaction();
	void remove_formaction(unsigned int pos);
	void set_current_formaction(unsigned int pos);
	unsigned int Formaction_stack_size()
	{
		return Formaction_stack.size();
	}
	char confirm(const std::string& prompt, const std::string& charset);

	void push_itemlist(unsigned int pos);
	void push_itemlist(std::shared_ptr<RssFeed> feed);
	void push_itemView(std::shared_ptr<RssFeed> f,
		const std::string& guid,
		const std::string& searchphrase = "");
	void push_help();
	void push_urlView(const std::vector<linkpair>& links,
		std::shared_ptr<RssFeed>& feed);
	void push_searchresult(std::shared_ptr<RssFeed> feed,
		const std::string& phrase = "");
	void View_dialogs();

	std::string run_filebrowser(const std::string& default_filename = "",
		const std::string& dir = "");
	std::string select_tag();
	std::string select_filter(
		const std::vector<filter_name_expr_pair>& filters);

	void open_in_browser(const std::string& url);
	void open_in_pager(const std::string& filename);

	std::string get_filename_suggestion(const std::string& s);

	bool get_next_unread(ItemListFormaction* itemlist,
		ItemViewFormaction* itemView = nullptr);
	bool get_previous_unread(ItemListFormaction* itemlist,
		ItemViewFormaction* itemView = nullptr);
	bool get_next(ItemListFormaction* itemlist,
		ItemViewFormaction* itemView = nullptr);
	bool get_previous(ItemListFormaction* itemlist,
		ItemViewFormaction* itemView = nullptr);
	bool get_random_unread(ItemListFormaction* itemlist,
		ItemViewFormaction* itemView = nullptr);

	bool get_next_unread_feed(ItemListFormaction* itemlist);
	bool get_prev_unread_feed(ItemListFormaction* itemlist);
	bool get_next_feed(ItemListFormaction* itemlist);
	bool get_prev_feed(ItemListFormaction* itemlist);

	void prepare_query_feed(std::shared_ptr<RssFeed> feed);

	void force_redraw();

	void set_colors(std::map<std::string, std::string>& fg_colors,
		std::map<std::string, std::string>& bg_colors,
		std::map<std::string, std::vector<std::string>>& attributes);

	void notify_itemlist_change(std::shared_ptr<RssFeed> feed);

	void feedlist_mark_pos_if_visible(unsigned int pos);

	void set_RegexManager(RegexManager* r);

	std::vector<std::pair<unsigned int, std::string>>
	get_formaction_names();

	std::shared_ptr<Formaction> get_current_formaction();

	std::shared_ptr<Formaction> get_formaction(unsigned int idx) const
	{
		return Formaction_stack[idx];
	}

	void goto_next_dialog();
	void goto_prev_dialog();

	void apply_colors_to_all_formactions();

	void update_bindings();

	void inside_qna(bool f);
	void inside_cmdline(bool f);

	void dump_current_form();

	static void ctrl_c_action(int sig);

protected:
	void set_bindings(std::shared_ptr<Formaction> fa);
	void apply_colors(std::shared_ptr<Formaction> fa);

	void handle_cmdline_completion(std::shared_ptr<Formaction> fa);
	void clear_line(std::shared_ptr<Formaction> fa);
	void clear_eol(std::shared_ptr<Formaction> fa);
	void cancel_input(std::shared_ptr<Formaction> fa);
	void delete_word(std::shared_ptr<Formaction> fa);

	Controller* ctrl;

	ConfigContainer* cfg;
	Keymap* keys;
	std::mutex mtx;

	friend class ColorManager;

	std::vector<std::shared_ptr<Formaction>> Formaction_stack;
	unsigned int current_formaction;

	std::vector<std::string> tags;

	RegexManager* rxman;

	std::map<std::string, std::string> fg_colors;
	std::map<std::string, std::string> bg_colors;
	std::map<std::string, std::vector<std::string>> attributes;

	bool is_inside_qna;
	bool is_inside_cmdline;

	std::string last_fragment;
	unsigned int tab_count;
	std::vector<std::string> suggestions;
};

} // namespace newsboat

#endif /* NEWSBOAT_VIEW_H_ */

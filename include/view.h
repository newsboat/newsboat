#ifndef NEWSBOAT_VIEW_H_
#define NEWSBOAT_VIEW_H_

#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

#include "3rd-party/optional.hpp"

#include "htmlrenderer.h"
#include "statusline.h"

namespace newsboat {

class Cache;
class ColorManager;
class Controller;
class ConfigContainer;
class FeedListFormAction;
class FilterContainer;
struct FilterNameExprPair;
class FormAction;
class ItemListFormAction;
class ItemViewFormAction;
class KeyMap;
struct MacroCmd;
class RegexManager;
class RssFeed;

class View : public IStatus {
public:
	explicit View(Controller*);
	~View();
	int run();
	std::string run_modal(std::shared_ptr<FormAction> f,
		const std::string& value = "");

	void set_feedlist(std::vector<std::shared_ptr<RssFeed>> feeds);
	void update_visible_feeds(std::vector<std::shared_ptr<RssFeed>> feeds);
	void set_keymap(KeyMap* k);
	void set_config_container(ConfigContainer* cfgcontainer);
	StatusLine& get_statusline();
	Controller* get_ctrl()
	{
		return ctrl;
	}
	ConfigContainer* get_cfg()
	{
		return cfg;
	}
	KeyMap* get_keymap()
	{
		return keys;
	}
	const KeyMap* get_keymap() const
	{
		return keys;
	}
	void set_tags(const std::vector<std::string>& t);
	void drop_queued_input();
	void pop_current_formaction();
	void remove_formaction(unsigned int pos);
	void set_current_formaction(unsigned int pos);
	unsigned int formaction_stack_size()
	{
		return formaction_stack.size();
	}
	char confirm(const std::string& prompt, const std::string& charset);

	void push_itemlist(unsigned int pos);
	std::shared_ptr<ItemListFormAction> push_itemlist(std::shared_ptr<RssFeed>
		feed);
	void push_itemview(std::shared_ptr<RssFeed> f,
		const std::string& guid,
		const std::string& searchphrase = "");
	void push_empty_formaction();
	void push_help();
	void push_urlview(const std::vector<LinkPair>& links,
		std::shared_ptr<RssFeed>& feed);
	void push_searchresult(std::shared_ptr<RssFeed> feed,
		const std::string& phrase = "");
	void view_dialogs();

	std::string run_filebrowser(const std::string& default_filename = "");
	std::string run_dirbrowser();
	std::string select_tag(const std::string& current_tag);
	std::string select_filter(
		const std::vector<FilterNameExprPair>& filters);

	nonstd::optional<std::uint8_t> open_in_browser(const std::string& url,
		const std::string& feedurl, const std::string& type, const std::string& title,
		bool interactive);
	void open_in_pager(const std::string& filename);

	std::string get_filename_suggestion(const std::string& s);

	bool get_next_unread(ItemListFormAction& itemlist,
		ItemViewFormAction* itemview = nullptr);
	bool get_previous_unread(ItemListFormAction& itemlist,
		ItemViewFormAction* itemview = nullptr);
	bool get_next(ItemListFormAction& itemlist,
		ItemViewFormAction* itemview = nullptr);
	bool get_previous(ItemListFormAction& itemlist,
		ItemViewFormAction* itemview = nullptr);
	bool get_random_unread(ItemListFormAction& itemlist,
		ItemViewFormAction* itemview = nullptr);

	bool get_next_unread_feed(ItemListFormAction& itemlist);
	bool get_prev_unread_feed(ItemListFormAction& itemlist);
	bool get_next_feed(ItemListFormAction& itemlist);
	bool get_prev_feed(ItemListFormAction& itemlist);

	void prepare_query_feed(std::shared_ptr<RssFeed> feed);

	void force_redraw();

	void notify_itemlist_change(std::shared_ptr<RssFeed> feed);

	void feedlist_mark_pos_if_visible(unsigned int pos);

	void set_cache(Cache* c);

	std::vector<std::pair<unsigned int, std::string>> get_formaction_names();

	std::shared_ptr<FormAction> get_current_formaction();

	std::shared_ptr<FormAction> get_formaction(unsigned int idx) const
	{
		return formaction_stack[idx];
	}

	void goto_next_dialog();
	void goto_prev_dialog();

	void apply_colors_to_all_formactions();

	void inside_qna(bool f);
	void inside_cmdline(bool f);

	static void ctrl_c_action(int sig);

protected:
	bool run_commands(const std::vector<MacroCmd>& commands);
	void run_macro(const std::string& event);

	void apply_colors(std::shared_ptr<FormAction> fa);

	void handle_cmdline_completion(std::shared_ptr<FormAction> fa);
	void clear_line(std::shared_ptr<FormAction> fa);
	void clear_eol(std::shared_ptr<FormAction> fa);
	void delete_word(std::shared_ptr<FormAction> fa);
	bool handle_qna_event(const std::string& event, std::shared_ptr<FormAction> fa);
	void handle_resize();

	Controller* ctrl;

	ConfigContainer* cfg;
	KeyMap* keys;
	std::mutex mtx;

	std::vector<std::shared_ptr<FormAction>> formaction_stack;
	unsigned int current_formaction;
	std::shared_ptr<FeedListFormAction> feedlist_form;

	void set_status(const std::string& msg) override;
	void show_error(const std::string& msg) override;
	StatusLine status_line;

	std::vector<std::string> tags;

	RegexManager& rxman;

	bool is_inside_qna;
	bool is_inside_cmdline;

	std::string last_fragment;
	unsigned int tab_count;
	Cache* rsscache;
	FilterContainer& filters;
	const ColorManager& colorman;
	std::vector<std::string> suggestions;

private:
	bool try_prepare_query_feed(std::shared_ptr<RssFeed> feed);
};

} // namespace newsboat

#endif /* NEWSBOAT_VIEW_H_ */

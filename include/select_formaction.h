#ifndef NEWSBEUTER_select_formaction__H
#define NEWSBEUTER_select_formaction__H

#include <formaction.h>
#include <filtercontainer.h>

namespace newsbeuter {

class select_formaction : public formaction {
	public:
		enum { SELECTTAG, SELECTFILTER };

		select_formaction(view *, std::string formstr);
		virtual ~select_formaction();
		virtual void prepare();
		virtual void init();
		virtual keymap_hint_entry * get_keymap_hint();
		inline std::string get_selected_value() { return value; }
		inline void set_tags(const std::vector<std::string>& t) { tags = t; }
		inline void set_filters(const std::vector<filter_name_expr_pair>& ff) { filters = ff; }
		void set_type(int t) { type = t; }
		virtual void handle_cmdline(const std::string& cmd);
		virtual std::string id() const { return (type == SELECTTAG) ? "tagselection" : "filterselection"; }
		virtual std::string title();
	private:
		virtual void process_operation(operation op, bool automatic = false, std::vector<std::string> * args = NULL);
		bool quit;
		int type;
		std::string value;
		std::vector<std::string> tags;
		std::vector<filter_name_expr_pair> filters;
};

}

#endif

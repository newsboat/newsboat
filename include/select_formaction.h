#ifndef NEWSBEUTER_select_formaction__H
#define NEWSBEUTER_select_formaction__H

#include <formaction.h>
#include <filtercontainer.h>

namespace newsbeuter {

class select_formaction : public formaction {
	public:
		enum class selection_type { TAG, FILTER };

		select_formaction(view *, std::string formstr);
		~select_formaction() override;
		void prepare() override;
		void init() override;
		keymap_hint_entry * get_keymap_hint() override;
		inline std::string get_selected_value() {
			return value;
		}
		inline void set_tags(const std::vector<std::string>& t) {
			tags = t;
		}
		inline void set_filters(const std::vector<filter_name_expr_pair>& ff) {
			filters = ff;
		}
		void set_type(selection_type t) {
			type = t;
		}
		void handle_cmdline(const std::string& cmd) override;
		std::string id() const override {
			return (type == selection_type::TAG) ? "tagselection" : "filterselection";
		}
		std::string title() override;
	private:
		void process_operation(operation op, bool automatic = false, std::vector<std::string> * args = nullptr) override;
		bool quit;
		selection_type type;
		std::string value;
		std::vector<std::string> tags;
		std::vector<filter_name_expr_pair> filters;
};

}

#endif

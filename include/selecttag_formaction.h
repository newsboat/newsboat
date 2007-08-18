#ifndef NEWSBEUTER_SELECTTAG_FORMACTION__H
#define NEWSBEUTER_SELECTTAG_FORMACTION__H

#include <formaction.h>

namespace newsbeuter {

class selecttag_formaction : public formaction {
	public:
		enum { SELECTTAG, SELECTFILTER };

		selecttag_formaction(view *, std::string formstr);
		virtual ~selecttag_formaction();
		virtual void prepare();
		virtual void init();
		virtual keymap_hint_entry * get_keymap_hint();
		inline std::string get_value() { return value; }
		inline void set_tags(const std::vector<std::string>& t) { tags = t; }
		inline void set_filters(const std::vector<std::pair<std::string, std::string> >& f) { filters = f; }
		void set_type(int t) { type = t; }
		virtual void handle_cmdline(const std::string& cmd);
	private:
		virtual void process_operation(operation op);
		bool quit;
		int type;
		std::string value;
		std::vector<std::string> tags;
		std::vector<std::pair<std::string, std::string> > filters;
};

}

#endif

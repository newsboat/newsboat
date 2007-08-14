#ifndef NEWSBEUTER_SELECTTAG_FORMACTION__H
#define NEWSBEUTER_SELECTTAG_FORMACTION__H

#include <formaction.h>

namespace newsbeuter {

class selecttag_formaction : public formaction {
	public:
		selecttag_formaction(view *, std::string formstr);
		virtual ~selecttag_formaction();
		virtual void prepare();
		virtual void init();
		virtual keymap_hint_entry * get_keymap_hint();
		inline std::string get_tag() { return tag; }
		inline void set_tags(const std::vector<std::string>& t) { tags = t; }
	private:
		virtual void process_operation(operation op);
		bool quit;
		std::string tag;
		std::vector<std::string> tags;
};

}

#endif

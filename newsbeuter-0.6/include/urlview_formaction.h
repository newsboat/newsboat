#ifndef NEWSBEUTER_URLVIEW_FORMACTION__H
#define NEWSBEUTER_URLVIEW_FORMACTION__H

#include <formaction.h>
#include <htmlrenderer.h>

namespace newsbeuter {

class urlview_formaction : public formaction {
	public:
		urlview_formaction(view *, std::string formstr);
		virtual ~urlview_formaction();
		virtual void prepare();
		virtual void init();
		virtual keymap_hint_entry * get_keymap_hint();
		inline void set_links(const std::vector<linkpair>& l) { links = l; }
	private:
		virtual void process_operation(operation op);
		std::vector<linkpair> links;
		bool quit;
};

}

#endif

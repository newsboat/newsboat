#ifndef NEWSBOAT_URLVIEW_FORMACTION_H_
#define NEWSBOAT_URLVIEW_FORMACTION_H_

#include <formaction.h>
#include <htmlrenderer.h>

namespace newsboat {

class urlview_formaction : public formaction {
	public:
		urlview_formaction(view *, std::shared_ptr<rss_feed>& feed, std::string formstr);
		virtual ~urlview_formaction();
		virtual void prepare();
		virtual void init();
		virtual keymap_hint_entry * get_keymap_hint();
		inline void set_links(const std::vector<linkpair>& l) {
			links = l;
		}
		virtual std::string id() const {
			return "urlview";
		}
		virtual std::string title();
		virtual void handle_cmdline(const std::string& cmd);
	private:
		virtual void process_operation(operation op, bool automatic = false, std::vector<std::string> * args = nullptr);
		std::vector<linkpair> links;
		bool quit;
		std::shared_ptr<rss_feed> feed;
};

}

#endif /* NEWSBOAT_URLVIEW_FORMACTION_H_ */

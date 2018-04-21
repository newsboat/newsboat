#ifndef NEWSBOAT_URLVIEW_FORMACTION_H_
#define NEWSBOAT_URLVIEW_FORMACTION_H_

#include "formaction.h"
#include "htmlrenderer.h"

namespace newsboat {

class urlview_formaction : public formaction {
	public:
		urlview_formaction(view *, std::shared_ptr<rss_feed>& feed, std::string formstr);
		~urlview_formaction() override;
		void prepare() override;
		void init() override;
		keymap_hint_entry * get_keymap_hint() override;
		void set_links(const std::vector<linkpair>& l) {
			links = l;
		}
		std::string id() const override {
			return "urlview";
		}
		std::string title() override;
		void handle_cmdline(const std::string& cmd) override;
	private:
		void process_operation(operation op, bool automatic = false, std::vector<std::string> * args = nullptr) override;
		std::vector<linkpair> links;
		bool quit;
		std::shared_ptr<rss_feed> feed;
};

}

#endif /* NEWSBOAT_URLVIEW_FORMACTION_H_ */

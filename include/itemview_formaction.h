#ifndef NEWSBEUTER_ITEMVIEW_FORMACTION__H
#define NEWSBEUTER_ITEMVIEW_FORMACTION__H

#include <formaction.h>
#include <htmlrenderer.h>
#include <rss.h>

namespace newsbeuter {

class itemview_formaction : public formaction {
	public:
		itemview_formaction(view *, std::string formstr);
		virtual ~itemview_formaction();
		virtual void prepare();
		virtual void init();
		inline void set_guid(const std::string& guid_) { guid = guid_; }
		inline void set_feed(rss_feed * fd) { feed = fd; }
		keymap_hint_entry * get_keymap_hint();
		virtual void handle_cmdline(const std::string& cmd);

		virtual void finished_qna(operation op);

		std::vector<std::string> render_html(const std::string& source, std::vector<linkpair>& links, const std::string& feedurl, unsigned int render_width);

	private:
		virtual void process_operation(operation op);
		void set_head(const std::string& s);

		void render_source(std::vector<std::string>& lines, std::string desc, unsigned int width);

		std::string guid;
		rss_feed * feed;
		bool show_source;
		std::vector<linkpair> links;
		bool quit;
};

}

#endif

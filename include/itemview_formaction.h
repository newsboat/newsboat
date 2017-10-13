#ifndef NEWSBEUTER_ITEMVIEW_FORMACTION__H
#define NEWSBEUTER_ITEMVIEW_FORMACTION__H

#include <formaction.h>
#include <htmlrenderer.h>
#include <textformatter.h>
#include <regexmanager.h>
#include <rss.h>

namespace newsboat {

class itemlist_formaction;

class itemview_formaction : public formaction {
	public:
		itemview_formaction(view *, std::shared_ptr<itemlist_formaction> il, std::string formstr);
		virtual ~itemview_formaction();
		virtual void prepare();
		virtual void init();
		inline void set_guid(const std::string& guid_) {
			guid = guid_;
		}
		inline void set_feed(std::shared_ptr<rss_feed> fd) {
			feed = fd;
		}
		void set_highlightphrase(const std::string& text);
		keymap_hint_entry * get_keymap_hint();
		virtual void handle_cmdline(const std::string& cmd);

		virtual std::string id() const {
			return "article";
		}
		virtual std::string title();

		virtual void finished_qna(operation op);

		std::vector<std::pair<LineType, std::string>>
			render_html(
				const std::string& source,
				std::vector<linkpair>& thelinks,
				const std::string& url);

		void set_regexmanager(regexmanager * r);

		void update_percent();

	private:
		virtual void process_operation(operation op, bool automatic = false, std::vector<std::string> * args = nullptr);
		void set_head(const std::string& s, const std::string& feedtitle, unsigned int unread, unsigned int total);
		void highlight_text(const std::string& searchphrase);

		void render_source(
				std::vector<std::pair<LineType, std::string>>& lines,
				std::string source);

		void do_search();

		std::string guid;
		std::shared_ptr<rss_feed> feed;
		bool show_source;
		std::vector<linkpair> links;
		bool quit;
		regexmanager * rxman;
		unsigned int num_lines;
		std::shared_ptr<itemlist_formaction> itemlist;
		bool in_search;
};

}

#endif

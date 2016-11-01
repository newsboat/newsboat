#ifndef NEWSBEUTER_ITEMLIST_FORMACTION__H
#define NEWSBEUTER_ITEMLIST_FORMACTION__H

#include <list_formaction.h>
#include <history.h>
#include <regexmanager.h>
#include <view.h>
#include <assert.h>
#include <listformatter.h>

namespace newsbeuter {

typedef std::pair<std::shared_ptr<rss_item>, unsigned int> itemptr_pos_pair;

enum class InvalidationMode {PARTIAL, COMPLETE};

class itemlist_formaction : public list_formaction {
	public:
		itemlist_formaction(view *, std::string formstr);
		virtual ~itemlist_formaction();
		virtual void prepare();
		virtual void init();

		virtual void set_redraw(bool b) {
			formaction::set_redraw(b);
			apply_filter = !(v->get_cfg()->get_configvalue_as_bool("show-read-articles"));
			invalidate(InvalidationMode::COMPLETE);
		}

		void set_feed(std::shared_ptr<rss_feed> fd);

		virtual std::string id() const {
			return "articlelist";
		}
		virtual std::string title();

		inline std::shared_ptr<rss_feed> get_feed() {
			return feed;
		}
		inline void set_pos(unsigned int p) {
			pos = p;
		}
		std::string get_guid();
		virtual keymap_hint_entry * get_keymap_hint();

		bool jump_to_next_unread_item(bool start_with_first);
		bool jump_to_previous_unread_item(bool start_with_last);
		bool jump_to_next_item(bool start_with_first);
		bool jump_to_previous_item(bool start_with_last);
		bool jump_to_random_unread_item();

		virtual void handle_cmdline(const std::string& cmd);

		void do_update_visible_items();

		virtual void finished_qna(operation op);

		inline void set_show_searchresult(bool b) {
			show_searchresult = b;
		}
		inline void set_searchphrase(const std::string& s) {
			searchphrase = s;
		}

		virtual void recalculate_form();

		void set_regexmanager(regexmanager * r);

	private:
		virtual void process_operation(operation op, bool automatic = false, std::vector<std::string> * args = nullptr);
		void set_head(const std::string& s, unsigned int unread, unsigned int total, const std::string &url);
		int get_pos(unsigned int idx);

		void save_article(const std::string& filename, std::shared_ptr<rss_item> item);

		void save_filterpos();

		void qna_end_setfilter();
		void qna_end_editflags();
		void qna_start_search();

		void handle_cmdline_num(unsigned int idx);

		std::string gen_flags(std::shared_ptr<rss_item> item);
		std::string gen_datestr(time_t t, const char * datetimeformat);

		void prepare_set_filterpos();

		inline void invalidate(InvalidationMode m) {
			assert(m == InvalidationMode::COMPLETE);

			invalidated = true;
			invalidation_mode = InvalidationMode::COMPLETE;
		}

		inline void invalidate(const unsigned int pos) {
			if (invalidated == true && invalidation_mode == InvalidationMode::COMPLETE)
				return;

			invalidated = true;
			invalidation_mode = InvalidationMode::PARTIAL;
			invalidated_itempos.push_back(pos);
		}

		std::string item2formatted_line(const itemptr_pos_pair& item,
		    const unsigned int width, const std::string& itemlist_format,
		    const std::string& datetime_format);

		unsigned int pos;
		std::shared_ptr<rss_feed> feed;
		bool apply_filter;
		matcher m;
		std::vector<itemptr_pos_pair> visible_items;
		bool show_searchresult;
		std::string searchphrase;

		history filterhistory;

		std::shared_ptr<rss_feed> search_dummy_feed;

		std::mutex redraw_mtx;

		bool set_filterpos;
		unsigned int filterpos;

		regexmanager * rxman;

		unsigned int old_width;
		int old_itempos;
		std::string old_sort_order;

		bool invalidated;
		InvalidationMode invalidation_mode;
		std::vector<unsigned int> invalidated_itempos;

		listformatter listfmt;
};

}

#endif

#ifndef NEWSBOAT_SEARCHRESULTSLISTFORMACTION_H_
#define NEWSBOAT_SEARCHRESULTSLISTFORMACTION_H_

#include <stack>

#include "configcontainer.h"
#include "itemlistformaction.h"
#include "regexmanager.h"
#include "rssfeed.h"
#include "view.h"
#include "keymap.h"
#include "fmtstrformatter.h"
#include "utf8string.h"

namespace newsboat {

struct SearchResult {
	std::shared_ptr<RssFeed> search_result_feed;
	Utf8String search_phrase;
};

class SearchResultsListFormAction : public ItemListFormAction {
public:
	SearchResultsListFormAction(View* vv,
		Utf8String formstr,
		Cache* cc,
		FilterContainer& f,
		ConfigContainer* cfg,
		RegexManager& r);

	Utf8String id() const override
	{
		return "searchresultslist";
	}

	void set_searchphrase(const Utf8String& s)
	{
		search_phrase = s;
	}

	const std::vector<KeyMapHintEntry>& get_keymap_hint() const override;

	void add_to_history(const std::shared_ptr<RssFeed>& feed, const Utf8String& str);

	void set_head(const Utf8String& s,
		unsigned int unread,
		unsigned int total,
		const Utf8String& url) override;

	Utf8String title() override;

protected:
	FmtStrFormatter setup_head_formatter(const Utf8String& s,
		unsigned int unread,
		unsigned int total,
		const Utf8String& url) override;

	bool process_operation(Operation op,
		bool automatic = false,
		std::vector<Utf8String>* args = nullptr) override;

private:
	std::stack<SearchResult> search_results;
	Utf8String search_phrase;
};

} // namespace newsboat

#endif /* NEWSBOAT_SEARCHRESULTSLISTFORMACTION_H_ */


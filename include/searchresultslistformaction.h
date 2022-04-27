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

namespace newsboat {

class SearchResultsListFormAction : public ItemListFormAction {
public:
	SearchResultsListFormAction(View* vv,
		std::string formstr,
		Cache* cc,
		FilterContainer& f,
		ConfigContainer* cfg,
		RegexManager& r);

	std::string id() const override
	{
		return "searchresultslist";
	}

	void set_searchphrase(const std::string& s)
	{
		search_phrase = s;
	}

	const std::vector<KeyMapHintEntry>& get_keymap_hint() const override;

	void add_to_history(const std::shared_ptr<RssFeed>& feed, const std::string& str);

	void set_head(const std::string& s,
		unsigned int unread,
		unsigned int total,
		const std::string& url) override;

	std::string title() override;

protected:
	bool process_operation(Operation op,
		bool automatic = false,
		std::vector<std::string>* args = nullptr) override;

private:
	std::stack<std::shared_ptr<RssFeed>> searchresultshistory;
	std::string search_phrase;
};

} // namespace newsboat

#endif /* NEWSBOAT_SEARCHRESULTSLISTFORMACTION_H_ */


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

namespace Newsboat {

struct SearchResult {
	std::shared_ptr<RssFeed> search_result_feed;
	std::string search_phrase;
};

class SearchResultsListFormAction : public ItemListFormAction {
public:
	SearchResultsListFormAction(View& vv,
		std::string formstr,
		Cache* cc,
		FilterContainer& f,
		ConfigContainer* cfg,
		RegexManager& r);
	~SearchResultsListFormAction() override = default;

	std::string id() const override
	{
		return "searchresultslist";
	}

	void set_searchphrase(const std::string& s)
	{
		search_phrase = s;
	}

	std::vector<KeyMapHintEntry> get_keymap_hint() const override;

	void add_to_history(const std::shared_ptr<RssFeed>& feed, const std::string& str);

	void set_head(const std::string& s,
		unsigned int unread,
		unsigned int total,
		const std::string& url) override;

	std::string title() override;

protected:
	FmtStrFormatter setup_head_formatter(const std::string& s,
		unsigned int unread,
		unsigned int total,
		const std::string& url) override;

	bool process_operation(Operation op,
		const std::vector<std::string>& args,
		BindingType bindingType = BindingType::BindKey) override;

private:
	std::stack<SearchResult> search_results;
	std::string search_phrase;
};

} // namespace Newsboat

#endif /* NEWSBOAT_SEARCHRESULTSLISTFORMACTION_H_ */


#ifndef NEWSBOAT_SEARCHRESULTSFORMACTION_H_
#define NEWSBOAT_SEARCHRESULTSFORMACTION_H_
#include "configcontainer.h"
#include "itemlistformaction.h"
#include "logger.h"
#include "regexmanager.h"
#include "rssfeed.h"
#include "view.h"
#include "keymap.h"
namespace newsboat {

class SearchResultsFormAction : public ItemListFormAction {
public:
	SearchResultsFormAction(View* vv,
		std::string formstr,
		Cache* cc,
		FilterContainer& f,
		ConfigContainer* cfg,
		RegexManager& r)
		: ItemListFormAction(
			  vv,
			  formstr,
			  cc,
			  f,
			  cfg,
			  r) {};

	std::string id() const override
	{
		return "searchresultslist";
	}

	const std::vector<KeyMapHintEntry>& get_keymap_hint() const override
	{
		static const std::vector<KeyMapHintEntry> hints = {
			{OP_OPEN, _("Open")},
			{OP_PREVSEARCHRESULTS, _("Prev search results")},
			{OP_NEXTSEARCHRESULTS, _("Next search results")}
		};
		return hints;
	}

	void add_history(const std::shared_ptr<RssFeed>& feed)
	{
		searchresultshistory.push_back(feed);
		searchistorypos++;
	}
protected:
	bool process_operation(Operation op,
		bool automatic = false,
		std::vector<std::string>* args = nullptr) override
	{
		switch (op) {
		case OP_NEXTSEARCHRESULTS:
			if (searchistorypos != searchresultshistory.size() - 1) {
				this->set_feed(searchresultshistory[++searchistorypos]);
			}
			break;
		case OP_PREVSEARCHRESULTS:
			if (searchistorypos > 0) {
				this->set_feed(searchresultshistory[--searchistorypos]);
			}
			break;
		default:
			ItemListFormAction::process_operation(op, automatic, args);
		}
		return true;
	}
private:
	std::vector<std::shared_ptr<RssFeed>> searchresultshistory;
	unsigned int searchistorypos = 0;
};
}
#endif


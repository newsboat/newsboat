#include "searchresultsformaction.h"

namespace newsboat {
SearchResultsFormAction::SearchResultsFormAction(View* vv,
	std::string formstr,
	Cache* cc,
	FilterContainer& f,
	ConfigContainer* cfg,
	RegexManager& r)
	: ItemListFormAction(vv, formstr, cc, f, cfg, r)
	, searchistorypos(0) {};

const std::vector<KeyMapHintEntry>& SearchResultsFormAction::get_keymap_hint() const
{
	static const std::vector<KeyMapHintEntry> hints = {
		{OP_OPEN, _("Open")},
		{OP_PREVSEARCHRESULTS, _("Prev search results")}
	};
	return hints;
};

void SearchResultsFormAction::add_history(const std::shared_ptr<RssFeed>& feed)
{
	searchresultshistory.push_back(feed);
	searchistorypos++;
}

bool SearchResultsFormAction::process_operation(
	Operation op,
	bool automatic,
	std::vector<std::string>* args)
{
	switch (op) {
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

} // namespace newsboat

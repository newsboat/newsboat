#include "searchresultslistformaction.h"

namespace newsboat {
SearchResultsListFormAction::SearchResultsListFormAction(View* vv,
	std::string formstr,
	Cache* cc,
	FilterContainer& f,
	ConfigContainer* cfg,
	RegexManager& r)
	: ItemListFormAction(vv, formstr, cc, f, cfg, r) {};

const std::vector<KeyMapHintEntry>& SearchResultsListFormAction::get_keymap_hint() const
{
	static const std::vector<KeyMapHintEntry> hints = {
		{OP_QUIT, _("Quit")},
		{OP_OPEN, _("Open")},
		{OP_PREVSEARCHRESULTS, _("Prev search results")}
	};
	return hints;
};

void SearchResultsListFormAction::add_history(const std::shared_ptr<RssFeed>& feed)
{
	searchresultshistory.push(feed);
}

bool SearchResultsListFormAction::process_operation(
	Operation op,
	bool automatic,
	std::vector<std::string>* args)
{
	switch (op) {
	case OP_PREVSEARCHRESULTS:
		if (searchresultshistory.size() > 1) {
			searchresultshistory.pop();
			this->set_feed(searchresultshistory.top());
		}
		break;
	default:
		ItemListFormAction::process_operation(op, automatic, args);
	}
	return true;
}

} // namespace newsboat

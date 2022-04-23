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
		{OP_PREVSEARCHRESULTS, _("Prev search results")},
		{OP_SEARCH, _("Search")},
		{OP_HELP, _("Help")}
	};
	return hints;
};

void SearchResultsListFormAction::add_to_history(const std::shared_ptr<RssFeed>& feed,
	const std::string& str)
{
	this->set_feed(feed);
	searchresultshistory.push(feed);
	this->set_searchphrase(str);
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
		} else {
			v->get_statusline().show_message(_("Already in first search result."));
		}
		break;
	default:
		return ItemListFormAction::process_operation(op, automatic, args);
	}
	return true;
}

} // namespace newsboat

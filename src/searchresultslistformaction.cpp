#include "searchresultslistformaction.h"
#include "keymap.h"

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
	const unsigned int itempos = list.get_position();

	switch (op) {
	case OP_OPEN:
		LOG(Level::INFO, "SearchResultsListFormAction: opening item at pos `%u'", itempos);
		if (!visible_items.empty()) {
			// no need to mark item as read, the itemview already do
			// that
			old_itempos = itempos;
			v->push_itemview(feed,
				visible_items[itempos].first->guid(), search_phrase);
			invalidate(itempos);
		} else {
			v->get_statusline().show_error(
				_("No item selected!")); // should not happen
		}
		break;
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

void SearchResultsListFormAction::set_head(const std::string& s,
	unsigned int unread,
	unsigned int total,
	const std::string& url)
{
	std::string title;
	FmtStrFormatter fmt;

	setup_head_formatter(s, unread, total, url, fmt);

	const unsigned int width = utils::to_u(f.get("title:w"));
	title = fmt.do_format(
			cfg->get_configvalue("searchresult-title-format"),
			width);
	set_value("head", title);
}

} // namespace newsboat

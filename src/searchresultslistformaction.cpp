#include "searchresultslistformaction.h"

#include "keymap.h"
#include "logger.h"

namespace Newsboat {
SearchResultsListFormAction::SearchResultsListFormAction(View& vv,
	std::string formstr,
	Cache* cc,
	FilterContainer& f,
	ConfigContainer* cfg,
	RegexManager& r)
	: ItemListFormAction(vv, formstr, cc, f, cfg, r) {};

std::vector<KeyMapHintEntry> SearchResultsListFormAction::get_keymap_hint() const
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
	if (search_phrase != str) {
		this->set_feed(feed);
		search_results.push({feed, str});
		this->set_searchphrase(str);
	}
}

bool SearchResultsListFormAction::process_operation(
	Operation op,
	const std::vector<std::string>& args,
	BindingType bindingType)
{
	const unsigned int itempos = list.get_position();

	switch (op) {
	case OP_OPEN:
		LOG(Level::INFO, "SearchResultsListFormAction: opening item at pos `%u'", itempos);
		if (!visible_items.empty()) {
			// no need to mark item as read, the itemview already do
			// that
			old_itempos = itempos;
			v.push_itemview(feed,
				visible_items[itempos].first->guid(), search_phrase);
			invalidate(itempos);
		} else {
			v.get_statusline().show_error(
				_("No item selected!")); // should not happen
		}
		break;
	case OP_PREVSEARCHRESULTS:
		if (search_results.size() > 1) {
			search_results.pop();
			this->set_feed(search_results.top().search_result_feed);
			this->set_searchphrase(search_results.top().search_phrase);
		} else {
			v.get_statusline().show_message(_("Already in first search result."));
		}
		break;
	default:
		return ItemListFormAction::process_operation(op, args, bindingType);
	}
	return true;
}

void SearchResultsListFormAction::set_head(const std::string& s,
	unsigned int unread,
	unsigned int total,
	const std::string& url)
{
	FmtStrFormatter fmt = setup_head_formatter(s, unread, total, url);

	const unsigned int width = utils::to_u(f.get("title:w"));
	set_title(fmt.do_format(
			cfg->get_configvalue("searchresult-title-format"),
			width));
}

std::string SearchResultsListFormAction::title()
{
	return strprintf::fmt(_("Search Result - '%s'"), search_phrase);
}

FmtStrFormatter SearchResultsListFormAction::setup_head_formatter(
	const std::string& s,
	unsigned int unread,
	unsigned int total,
	const std::string& url)
{
	FmtStrFormatter fmt = ItemListFormAction::setup_head_formatter(s, unread, total, url);

	fmt.register_fmt('s', search_phrase);

	return fmt;
};


} // namespace Newsboat

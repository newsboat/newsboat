#include "dialog.h"

#include <map>
#include <utility>

namespace newsboat {

constexpr std::pair<Dialog, const char*> dialog_string_mapping[] {
	{ Dialog::Article, "article" },
	{ Dialog::ArticleList, "articlelist" },
	{ Dialog::DialogList, "dialogs" },
	{ Dialog::DirBrowser, "dirbrowser" },
	{ Dialog::Empty, "empty" },
	{ Dialog::FeedList,	"feedlist" },
	{ Dialog::FileBrowser, "filebrowser" },
	{ Dialog::FilterSelection, "filterselection" },
	{ Dialog::Help, "help" },
	{ Dialog::Podboat, "podboat" },
	{ Dialog::SearchResultsList, "searchresultslist" },
	{ Dialog::TagSelection, "tagselection" },
	{ Dialog::UrlView, "urlview" },
};

std::map<Dialog, std::string> name_from_dialog_map()
{
	std::map<Dialog, std::string> m;
	for (const auto& entry : dialog_string_mapping) {
		m.insert(std::make_pair(entry.first, entry.second));
	}
	return m;
}

std::map<std::string, Dialog> dialog_from_name_map()
{
	std::map<std::string, Dialog> m;
	for (const auto& entry : dialog_string_mapping) {
		m.insert(std::make_pair(entry.second, entry.first));
	}
	return m;
}

std::optional<Dialog> dialog_from_name(const std::string& name)
{
	static const auto m = dialog_from_name_map();
	const auto it = m.find(name);
	if (it != m.end()) {
		return it->second;
	}
	return std::nullopt;
}

std::string dialog_name(Dialog dialog)
{
	static const auto m = name_from_dialog_map();
	return m.at(dialog);
}

} // namespace newsboat

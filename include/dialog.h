#ifndef NEWSBOAT_DIALOG_H_
#define NEWSBOAT_DIALOG_H_

#include <optional>
#include <string>

namespace newsboat {

class AllDialogs {};

enum class Dialog {
	Article,
	ArticleList,
	DialogList,
	DirBrowser,
	Empty,
	FeedList,
	FileBrowser,
	FilterSelection,
	Help,
	Podboat,
	SearchResultsList,
	TagSelection,
	UrlView,
};

std::optional<Dialog> dialog_from_name(const std::string& name);
std::string dialog_name(Dialog dialog);

} // namespace newsboat

#endif /* NEWSBOAT_DIALOG_H_ */

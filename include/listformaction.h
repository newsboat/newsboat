#ifndef NEWSBOAT_LISTFORMACTION_H_
#define NEWSBOAT_LISTFORMACTION_H_

#include <cstdint>
#include <optional>

#include "formaction.h"
#include "listwidget.h"
#include "regexmanager.h"

namespace Newsboat {

class ListFormAction : public FormAction {
public:
	ListFormAction(View& v, const std::string& context, std::string formstr,
		std::string list_name,
		ConfigContainer* cfg, RegexManager& r);
	~ListFormAction() override = default;

protected:
	bool process_operation(Operation op,
		const std::vector<std::string>& args,
		BindingType bindingType = BindingType::BindKey) override;
	std::optional<std::uint8_t> open_unread_items_in_browser(
		std::shared_ptr<RssFeed> feed,
		bool markread);

	ListWidget list;
};

} // namespace Newsboat

#endif /* NEWSBOAT_LISTFORMACTION_H_ */

#ifndef NEWSBOAT_LISTFORMACTION_H_
#define NEWSBOAT_LISTFORMACTION_H_

#include <cstdint>

#include "3rd-party/optional.hpp"

#include "formaction.h"
#include "listwidget.h"
#include "utf8string.h"

namespace newsboat {

class ListFormAction : public FormAction {
public:
	ListFormAction(View*, Utf8String formstr, Utf8String list_name,
		ConfigContainer* cfg);

protected:
	bool process_operation(Operation op,
		bool automatic = false,
		std::vector<Utf8String>* args = nullptr) override;
	nonstd::optional<std::uint8_t> open_unread_items_in_browser(
		std::shared_ptr<RssFeed> feed,
		bool markread);

	ListWidget list;
};

} // namespace newsboat

#endif /* NEWSBOAT_LISTFORMACTION_H_ */

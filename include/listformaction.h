#ifndef NEWSBOAT_LISTFORMACTION_H_
#define NEWSBOAT_LISTFORMACTION_H_

#include "formaction.h"

namespace newsboat {

class ListFormAction : public FormAction {
public:
	ListFormAction(View*, std::string formstr, ConfigContainer* cfg);

protected:
	void process_operation(Operation op,
		bool automatic = false,
		std::vector<std::string>* args = nullptr) override;
	void open_unread_items_in_browser(std::shared_ptr<RssFeed> feed,
		bool markread);
};

} // namespace newsboat

#endif /* NEWSBOAT_LISTFORMACTION_H_ */

#ifndef NEWSBOAT_LISTFORMACTION_H_
#define NEWSBOAT_LISTFORMACTION_H_

#include "formaction.h"

namespace newsboat {

class list_formaction : public formaction {
public:
	list_formaction(view*, std::string formstr);

protected:
	void process_operation(operation op,
		bool automatic = false,
		std::vector<std::string>* args = nullptr) override;
	void open_unread_items_in_browser(std::shared_ptr<rss_feed> feed,
		bool markread);
};

} // namespace newsboat

#endif /* NEWSBOAT_LISTFORMACTION_H_ */

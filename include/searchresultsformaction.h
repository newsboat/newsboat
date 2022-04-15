#ifndef NEWSBOAT_SEARCHRESULTSFORMACTION_H_
#define NEWSBOAT_SEARCHRESULTSFORMACTION_H_
#include "configcontainer.h"
#include "itemlistformaction.h"
#include "logger.h"
#include "regexmanager.h"
#include "rssfeed.h"
#include "view.h"
#include "keymap.h"

namespace newsboat {

class SearchResultsFormAction : public ItemListFormAction {
public:
	SearchResultsFormAction(View* vv,
		std::string formstr,
		Cache* cc,
		FilterContainer& f,
		ConfigContainer* cfg,
		RegexManager& r);

	std::string id() const override
	{
		return "searchresultslist";
	}

	const std::vector<KeyMapHintEntry>& get_keymap_hint() const override;
	void add_history(const std::shared_ptr<RssFeed>& feed);

protected:
	bool process_operation(Operation op,
		bool automatic = false,
		std::vector<std::string>* args = nullptr) override;

private:
	std::vector<std::shared_ptr<RssFeed>> searchresultshistory;
	unsigned int searchistorypos;
};

} // namespace newsboat

#endif /* NEWSBOAT_SEARCHRESULTSFORMACTION_H_ */


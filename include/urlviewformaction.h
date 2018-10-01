#ifndef NEWSBOAT_URLVIEWFORMACTION_H_
#define NEWSBOAT_URLVIEWFORMACTION_H_

#include "formaction.h"
#include "htmlrenderer.h"

namespace newsboat {

class UrlViewFormaction : public Formaction {
public:
	UrlViewFormaction(View*,
		std::shared_ptr<RssFeed>& feed,
		std::string formstr);
	~UrlViewFormaction() override;
	void prepare() override;
	void init() override;
	keymap_hint_entry* get_keymap_hint() override;
	void set_links(const std::vector<linkpair>& l)
	{
		links = l;
	}
	std::string id() const override
	{
		return "urlView";
	}
	std::string title() override;
	void handle_cmdline(const std::string& cmd) override;

private:
	void process_operation(operation op,
		bool automatic = false,
		std::vector<std::string>* args = nullptr) override;
	std::vector<linkpair> links;
	bool quit;
	std::shared_ptr<RssFeed> feed;
};

} // namespace newsboat

#endif /* NEWSBOAT_URLVIEWFORMACTION_H_ */

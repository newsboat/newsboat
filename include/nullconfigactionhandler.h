#ifndef NEWSBOAT_NULLCONFIGACTIONHANDLER_H_
#define NEWSBOAT_NULLCONFIGACTIONHANDLER_H_

#include "configactionhandler.h"

namespace Newsboat {

class NullConfigActionHandler : public ConfigActionHandler {
public:
	NullConfigActionHandler() {}
	~NullConfigActionHandler() override {}
	void handle_action(const std::string&,
		const std::vector<std::string>&) override
	{
	}
	void dump_config(std::vector<std::string>&) const override {}
};

} // namespace Newsboat

#endif /* NEWSBOAT_NULLCONFIGACTIONHANDLER_H_ */


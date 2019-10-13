#ifndef NEWSBOAT_NULLCONFIGACTIONHANDLER_H_
#define NEWSBOAT_NULLCONFIGACTIONHANDLER_H_

#include "configactionhandler.h"

namespace newsboat {

class NullConfigActionHandler : public ConfigActionHandler {
public:
	NullConfigActionHandler() {}
	~NullConfigActionHandler() override {}
	void handle_action(const std::string&,
		const std::vector<std::string>&) override
	{}
	void dump_config(std::vector<std::string>&) override {}
};

} // namespace newsboat

#endif /* NEWSBOAT_NULLCONFIGACTIONHANDLER_H_ */

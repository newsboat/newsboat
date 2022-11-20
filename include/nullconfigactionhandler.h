#ifndef NEWSBOAT_NULLCONFIGACTIONHANDLER_H_
#define NEWSBOAT_NULLCONFIGACTIONHANDLER_H_

#include "configactionhandler.h"
#include "utf8string.h"

namespace newsboat {

class NullConfigActionHandler : public ConfigActionHandler {
public:
	NullConfigActionHandler() {}
	~NullConfigActionHandler() override {}
	void handle_action(const Utf8String&,
		const std::vector<Utf8String>&) override
	{
	}
	void dump_config(std::vector<Utf8String>&) const override {}
};

} // namespace newsboat

#endif /* NEWSBOAT_NULLCONFIGACTIONHANDLER_H_ */


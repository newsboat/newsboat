#ifndef NEWSBOAT_CONFIGACTIONHANDLER_H_
#define NEWSBOAT_CONFIGACTIONHANDLER_H_

#include <vector>

#include "utf8string.h"

namespace newsboat {

class ConfigActionHandler {
public:
	// Derived classes can override this function to handle the `params` string without the default tokenization
	virtual void handle_action(const Utf8String& action,
		const Utf8String& params);

	virtual void dump_config(std::vector<Utf8String>& config_output) const = 0;
	ConfigActionHandler() {}
	virtual ~ConfigActionHandler() {}

private:
	// Derived classes should override either of the handle_action() overloads
	virtual void handle_action(const Utf8String& action,
		const std::vector<Utf8String>& params);
};

} // namespace newsboat

#endif /* NEWSBOAT_CONFIGACTIONHANDLER_H_ */


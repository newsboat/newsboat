#ifndef NEWSBOAT_CONFIGACTIONHANDLER_H_
#define NEWSBOAT_CONFIGACTIONHANDLER_H_

#include <string>
#include <vector>

namespace newsboat {

class ConfigActionHandler {
public:
	// Derived classes can override this function to handle the `params` string without the default tokenization
	virtual void handle_action(const std::string& action,
		const std::string& params);

	virtual void dump_config(std::vector<std::string>& config_output) const = 0;
	ConfigActionHandler() {}
	virtual ~ConfigActionHandler() {}

private:
	// Derived classes should override either of the handle_action() overloads
	virtual void handle_action(const std::string& action,
		const std::vector<std::string>& params);
};

} // namespace newsboat

#endif /* NEWSBOAT_CONFIGACTIONHANDLER_H_ */


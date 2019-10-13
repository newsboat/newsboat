#ifndef NEWSBOAT_CONFIGACTIONHANDLER_H_
#define NEWSBOAT_CONFIGACTIONHANDLER_H_

#include <string>
#include <vector>

namespace newsboat {

class ConfigActionHandler {
public:
	virtual void handle_action(const std::string& action,
		const std::vector<std::string>& params) = 0;
	virtual void dump_config(std::vector<std::string>& config_output) = 0;
	ConfigActionHandler() {}
	virtual ~ConfigActionHandler() {}
};

} // namespace newsboat

#endif /* NEWSBOAT_CONFIGACTIONHANDLER_H_ */

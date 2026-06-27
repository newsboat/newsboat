#ifndef NEWSBOAT_COLORMANAGER_H_
#define NEWSBOAT_COLORMANAGER_H_

#include <map>
#include <vector>

#include "configactionhandler.h"
#include "textstyle.h"

namespace podboat {
class PbView;
}

class View;

namespace newsboat {

class ConfigParser;

class ColorManager : public ConfigActionHandler {
public:
	~ColorManager() override = default;
	void register_commands(ConfigParser& cfgparser);
	void handle_action(std::string_view action,
		const std::vector<std::string>& params) override;
	void dump_config(std::vector<std::string>& config_output) const override;
	std::map<std::string, std::string> get_stfl_styles() const;

private:
	std::map<std::string, TextStyle> element_styles;
};

} // namespace newsboat

#endif /* NEWSBOAT_COLORMANAGER_H_ */

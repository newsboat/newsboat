#ifndef NEWSBOAT_COLORMANAGER_H_
#define NEWSBOAT_COLORMANAGER_H_

#include <map>
#include <vector>

#include "configparser.h"

namespace podboat {
class PbView;
}

class View;

namespace newsboat {

struct TextStyle {
	std::string fg_color;
	std::string bg_color;
	std::vector<std::string> attributes;
};

class ColorManager : public ConfigActionHandler {
public:
	ColorManager();
	~ColorManager() override;
	void register_commands(ConfigParser& cfgparser);
	void handle_action(const std::string& action,
		const std::vector<std::string>& params) override;
	void dump_config(std::vector<std::string>& config_output) override;
	void set_pb_colors(podboat::PbView* v);
	std::map<std::string, TextStyle> get_styles()
	{
		return element_styles;
	}

private:
	std::map<std::string, TextStyle> element_styles;
};

} // namespace newsboat

#endif /* NEWSBOAT_COLORMANAGER_H_ */

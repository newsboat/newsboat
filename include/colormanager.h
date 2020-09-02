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

class ColorManager : public ConfigActionHandler {
public:
	ColorManager();
	~ColorManager() override;
	void register_commands(ConfigParser& cfgparser);
	void handle_action(const std::string& action,
		const std::vector<std::string>& params) override;
	void dump_config(std::vector<std::string>& config_output) override;
	void set_pb_colors(podboat::PbView* v);
	std::map<std::string, std::string>& get_fgcolors()
	{
		return fg_colors;
	}
	std::map<std::string, std::string>& get_bgcolors()
	{
		return bg_colors;
	}
	std::map<std::string, std::vector<std::string>>& get_attributes()
	{
		return attributes;
	}

private:
	std::map<std::string, std::string> fg_colors;
	std::map<std::string, std::string> bg_colors;
	std::map<std::string, std::vector<std::string>> attributes;
};

} // namespace newsboat

#endif /* NEWSBOAT_COLORMANAGER_H_ */

#ifndef NEWSBOAT_COLORMANAGER_H_
#define NEWSBOAT_COLORMANAGER_H_

#include <map>
#include <vector>

#include "configparser.h"

namespace podboat {
class pb_view;
}

class view;

namespace newsboat {

class colormanager : public config_action_handler {
public:
	colormanager();
	~colormanager() override;
	void register_commands(configparser& cfgparser);
	void handle_action(const std::string& action,
		const std::vector<std::string>& params) override;
	void dump_config(std::vector<std::string>& config_output) override;
	bool colors_loaded()
	{
		return colors_loaded_;
	}
	void set_pb_colors(podboat::pb_view* v);
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
	bool colors_loaded_;
	std::map<std::string, std::string> fg_colors;
	std::map<std::string, std::string> bg_colors;
	std::map<std::string, std::vector<std::string>> attributes;
};

} // namespace newsboat

#endif /* NEWSBOAT_COLORMANAGER_H_ */

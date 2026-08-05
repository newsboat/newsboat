#ifndef NEWSBOAT_COLORMANAGER_H_
#define NEWSBOAT_COLORMANAGER_H_

#include <map>
#include <vector>

#include "configactionhandler.h"

#include "libnewsboat-ffi/src/colormanager.rs.h" // IWYU pragma: export

namespace podboat {
class PbView;
}

class View;

namespace newsboat {

class ConfigParser;

class ColorManager : public ConfigActionHandler {
public:
	ColorManager();
	~ColorManager() override = default;
	void register_commands(ConfigParser& cfgparser);
	void handle_action(std::string_view action,
		const std::vector<std::string>& params) override;
	void dump_config(std::vector<std::string>& config_output) const override;
	std::map<std::string, std::string> get_stfl_styles() const;

private:
	rust::Box<colormanager::bridged::ColorManager> rs_object;
};

} // namespace newsboat

#endif /* NEWSBOAT_COLORMANAGER_H_ */

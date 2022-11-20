#ifndef NEWSBOAT_COLORMANAGER_H_
#define NEWSBOAT_COLORMANAGER_H_

#include <functional>
#include <map>
#include <vector>

#include "configactionhandler.h"
#include "utf8string.h"

namespace podboat {
class PbView;
}

class View;

namespace newsboat {

class ConfigParser;

struct TextStyle {
	Utf8String fg_color;
	Utf8String bg_color;
	std::vector<Utf8String> attributes;
};

class ColorManager : public ConfigActionHandler {
public:
	ColorManager();
	~ColorManager() override;
	void register_commands(ConfigParser& cfgparser);
	void handle_action(const Utf8String& action,
		const std::vector<Utf8String>& params) override;
	void dump_config(std::vector<Utf8String>& config_output) const override;
	void apply_colors(std::function<void(const Utf8String&, const Utf8String&)>
		stfl_value_setter) const;

private:
	void emit_fallback_from_to(const Utf8String& from_element, const Utf8String& to_element,
		const std::function<void(const Utf8String&, const Utf8String&)>& stfl_value_setter)
	const;

	std::map<Utf8String, TextStyle> element_styles;
};

} // namespace newsboat

#endif /* NEWSBOAT_COLORMANAGER_H_ */

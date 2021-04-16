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
	std::string fg_color;
	std::string bg_color;
	std::vector<std::string> attributes;
};

struct InternalTextStyle {
	Utf8String fg_color;
	Utf8String bg_color;
	std::vector<Utf8String> attributes;
};

class ColorManager : public ConfigActionHandler {
public:
	ColorManager();
	~ColorManager() override;
	void register_commands(ConfigParser& cfgparser);
	void handle_action(const std::string& action,
		const std::vector<std::string>& params) override;
	void dump_config(std::vector<std::string>& config_output) const override;
	void apply_colors(std::function<void(const std::string&, const std::string&)>
		stfl_value_setter) const;
	std::map<std::string, TextStyle> get_styles() const
	{
		std::map<std::string, TextStyle> result;

		for (const auto& entry : element_styles) {
			const auto key = entry.first;
			const auto value = entry.second;

			TextStyle new_value;
			new_value.fg_color = value.fg_color.to_utf8();
			new_value.bg_color = value.bg_color.to_utf8();
			for (const auto& attr : value.attributes) {
				new_value.attributes.push_back(attr.to_utf8());
			}

			result[key.to_utf8()] = new_value;
		}

		return result;
	}

private:
	std::map<Utf8String, InternalTextStyle> element_styles;
};

} // namespace newsboat

#endif /* NEWSBOAT_COLORMANAGER_H_ */

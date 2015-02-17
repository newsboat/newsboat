#ifndef COLORMANAGER_H_
#define COLORMANAGER_H_

#include <configparser.h>
#include <vector>
#include <map>

namespace podbeuter {
	class pb_view;
}

class view;

namespace newsbeuter
{


class colormanager : public config_action_handler {
	public:
		colormanager();
		~colormanager();
		void register_commands(configparser& cfgparser);
		virtual void handle_action(const std::string& action, const std::vector<std::string>& params);
		virtual void dump_config(std::vector<std::string>& config_output);
		inline bool colors_loaded() { return colors_loaded_; }
		void set_pb_colors(podbeuter::pb_view * v);
		inline std::map<std::string,std::string>& get_fgcolors() { return fg_colors; }
		inline std::map<std::string,std::string>& get_bgcolors() { return bg_colors; }
		inline std::map<std::string,std::vector<std::string>>& get_attributes() { return attributes; }
	private:

		bool colors_loaded_;
		std::map<std::string,std::string> fg_colors;
		std::map<std::string,std::string> bg_colors;
		std::map<std::string,std::vector<std::string>> attributes;
};

}

#endif

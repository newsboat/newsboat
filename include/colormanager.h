#ifndef COLORMANAGER_H_
#define COLORMANAGER_H_

#include <view.h>
#include <configparser.h>
#include <vector>
#include <map>

namespace newsbeuter
{

class colormanager : public config_action_handler {
	public:
		colormanager();
		~colormanager();
		void register_commands(configparser& cfgparser);
		virtual action_handler_status handle_action(const std::string& action, const std::vector<std::string>& params);
		inline bool colors_loaded() { return colors_loaded_; }
		void set_colors(view * v);
	private:
		bool colors_loaded_;
		std::map<std::string,std::string> fg_colors;
		std::map<std::string,std::string> bg_colors;
		std::map<std::string,std::vector<std::string> > attributes;
};

}

#endif

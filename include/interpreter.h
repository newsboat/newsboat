#ifndef NEWSBEUTER_INTERPRETER__H
#define NEWSBEUTER_INTERPRETER__H

#include <string>
#include <view.h>
#include <controller.h>
#include <configparser.h>

#if EMBED_LUA

extern "C" {
#include <lua.h>
}

namespace newsbeuter {

	class script_interpreter : public config_action_handler {
		public:
			script_interpreter();
			virtual ~script_interpreter();
			void load_script(const std::string& path);
			void run_function(const std::string& name);
			virtual action_handler_status handle_action(const std::string& action, const std::vector<std::string>& params);

			inline void set_view(view * vv) { v = vv; }
			inline view * get_view() { return v; }

			inline void set_controller(controller * cc) { c = cc; }
			inline controller * get_controller() { return c; }

		private:
			lua_State * L;
			view * v;
			controller * c;
	};

script_interpreter * GetInterpreter();

}

#endif


#endif

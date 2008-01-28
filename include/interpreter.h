#ifndef NEWSBEUTER_INTERPRETER__H
#define NEWSBEUTER_INTERPRETER__H

#include <string>
#include <view.h>
#include <controller.h>
#include <configparser.h>
#if RUBY
#	include <ruby.h>
#endif

namespace newsbeuter {

	class script_interpreter : public config_action_handler {
		public:
			script_interpreter();
			virtual ~script_interpreter();
			void load_script(const std::string& path);
			void run_function(const std::string& name);
			virtual action_handler_status handle_action(const std::string& action, const std::vector<std::string>& params);

			void set_view(view * vv);
			inline view * get_view() { return v; }

			void set_controller(controller * cc);
			inline controller * get_controller() { return c; }

		private:
			view * v;
			controller * c;
#if RUBY
			VALUE rv;
			VALUE rc;
#endif
	};

#if RUBY
script_interpreter * GetInterpreter();
#endif

}

#endif

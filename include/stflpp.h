#ifndef STFLPP__H
#define STFLPP__H

#include <stfl.h>
#include <string>

namespace noos {

class stfl {
	public:
		stfl(const std::string& text);
		~stfl();

		std::string run(int timeout);

		std::string get(const std::string& name);
		void set(const std::string& name, const std::string& value);

		std::string get_focus();
		void set_focus(const std::string& name);

		std::string dump(const std::string& name, const std::string& prefix, int focus);
		void modify(const std::string& name, const std::string& mode, const std::string& text);
		std::string lookup(const std::string& path, const std::string& newname);

		static std::string error();
		static void error_action(const std::string& mode);

		static void reset();
		static std::string quote(const std::string& text);
	private:
		stfl_form * f;
};

}


#endif

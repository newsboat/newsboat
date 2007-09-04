#ifndef NEWSBEUTER_FILEBROWSER_FORMACTION__H
#define NEWSBEUTER_FILEBROWSER_FORMACTION__H

#include <formaction.h>

namespace newsbeuter {

enum filebrowser_type { FBT_OPEN, FBT_SAVE };

class filebrowser_formaction : public formaction {
	public:
		filebrowser_formaction(view *, std::string formstr);
		virtual ~filebrowser_formaction();
		virtual void prepare();
		virtual void init();
		virtual keymap_hint_entry * get_keymap_hint();

		inline void set_dir(const std::string& d) { dir = d; }
		inline void set_default_filename(const std::string& fn) { default_filename = fn; }
		inline void set_type(filebrowser_type t) { type = t; }

	private:
		virtual void process_operation(operation op);

		std::string add_file(std::string filename);
		std::string get_filename_suggestion(const std::string& s);
		std::string get_rwx(unsigned short val);

		bool quit;
		std::string cwd;
		std::string dir;
		std::string default_filename;
		filebrowser_type type;
};

}


#endif

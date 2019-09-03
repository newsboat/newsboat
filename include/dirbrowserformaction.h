#ifndef NEWSBOAT_DIRBROWSERFORMACTION_H
#define NEWSBOAT_DIRBROWSERFORMACTION_H

#include <sys/stat.h>
#include <grp.h>

#include "configcontainer.h"
#include "formaction.h"

namespace newsboat {

	class DirBrowserFormAction : public FormAction {
	public:
		DirBrowserFormAction(View*, std::string formstr, ConfigContainer* cfg);
		~DirBrowserFormAction() override;
		void prepare() override;
		void init() override;
		KeyMapHintEntry* get_keymap_hint() override;

		void set_dir(const std::string& d)
		{
			dir = d;
		}

		std::string id() const override
		{
			return "dirbrowser";
		}
		std::string title() override;

	private:
		void process_operation(Operation op,
							   bool automatic = false,
							   std::vector<std::string>* args = nullptr) override;

		std::string add_directory(std::string dirname);
		std::string get_rwx(unsigned short val);

		std::string get_owner(uid_t uid);
		std::string get_group(gid_t gid);
		std::string
		get_formatted_dirname(std::string dirname, char ftype, mode_t mode);

		std::string cwd;
		std::string dir;

	};

} // namespace newsboat

#endif //NEWSBOAT_DIRBROWSERFORMACTION_H

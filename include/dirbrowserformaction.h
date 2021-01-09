#ifndef NEWSBOAT_DIRBROWSERFORMACTION_H
#define NEWSBOAT_DIRBROWSERFORMACTION_H

#include <sys/stat.h>
#include <grp.h>

#include "configcontainer.h"
#include "listformatter.h"
#include "listwidget.h"
#include "formaction.h"

namespace newsboat {

class DirBrowserFormAction : public FormAction {
public:
	DirBrowserFormAction(View*, std::string formstr, ConfigContainer* cfg);
	~DirBrowserFormAction() override;
	void prepare() override;
	void init() override;
	KeyMapHintEntry* get_keymap_hint() override;

	std::string id() const override
	{
		return "dirbrowser";
	}
	std::string title() override;

private:
	bool process_operation(Operation op,
		bool automatic = false,
		std::vector<std::string>* args = nullptr) override;
	void update_title(const std::string& working_directory);

	void add_directory(ListFormatter& listfmt,
		std::vector<std::string>& id_at_position,
		std::string dirname);
	std::string get_rwx(unsigned short val);
	std::vector<std::string> id_at_position;

	std::string get_owner(uid_t uid);
	std::string get_group(gid_t gid);
	std::string get_formatted_dirname(std::string dirname, char ftype, mode_t mode);

	std::string cwd;
	std::string dir;

	ListWidget files_list;
};

} // namespace newsboat

#endif //NEWSBOAT_DIRBROWSERFORMACTION_H

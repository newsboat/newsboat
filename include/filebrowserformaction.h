#ifndef NEWSBOAT_FILEBROWSERFORMACTION_H_
#define NEWSBOAT_FILEBROWSERFORMACTION_H_

#include <sys/stat.h>
#include <grp.h>

#include "configcontainer.h"
#include "listformatter.h"
#include "listwidget.h"
#include "formaction.h"

namespace newsboat {

class FileBrowserFormAction : public FormAction {
public:
	FileBrowserFormAction(View*, std::string formstr, ConfigContainer* cfg);
	~FileBrowserFormAction() override;
	void prepare() override;
	void init() override;
	KeyMapHintEntry* get_keymap_hint() override;

	void set_default_filename(const std::string& fn)
	{
		default_filename = fn;
	}

	std::string id() const override
	{
		return "filebrowser";
	}
	std::string title() override;

private:
	bool process_operation(Operation op,
		bool automatic = false,
		std::vector<std::string>* args = nullptr) override;
	void update_title(const std::string& working_directory);

	void add_file(ListFormatter& listfmt,
		std::vector<std::string>& id_at_position,
		std::string filename);
	std::string get_filename_suggestion(const std::string& s);
	std::string get_rwx(unsigned short val);
	std::vector<std::string> id_at_position;

	char get_filetype(mode_t mode);
	std::string get_owner(uid_t uid);
	std::string get_group(gid_t gid);
	std::string get_formatted_filename(std::string filename, char ftype,
		mode_t mode);

	bool quit;
	std::string cwd;
	std::string dir;
	std::string default_filename;

	ListWidget files_list;
};

} // namespace newsboat

#endif /* NEWSBOAT_FILEBROWSERFORMACTION_H_ */

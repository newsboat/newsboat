#ifndef NEWSBOAT_FILEBROWSERFORMACTION_H_
#define NEWSBOAT_FILEBROWSERFORMACTION_H_

#include <sys/stat.h>
#include <grp.h>

#include "configcontainer.h"
#include "file_system.h"
#include "formaction.h"
#include "listformatter.h"
#include "listwidget.h"

namespace newsboat {

class FileBrowserFormAction : public FormAction {
public:
	FileBrowserFormAction(View*, std::string formstr, ConfigContainer* cfg);
	~FileBrowserFormAction() override;
	void prepare() override;
	void init() override;
	const std::vector<KeyMapHintEntry>& get_keymap_hint() const override;

	void set_default_filename(const std::string& fn)
	{
		default_filename = fn;
	}

	std::string id() const override
	{
		return "filebrowser";
	}
	std::string title() override;

protected:
	std::string main_widget() const override
	{
		return "filename";
	}

private:
	bool process_operation(Operation op, std::vector<std::string>* args = nullptr) override;
	void update_title(const std::string& working_directory);

	void add_file(ListFormatter& listfmt,
		std::vector<file_system::FileSystemEntry>& id_at_position,
		std::string filename);
	std::string get_filename_suggestion(const std::string& s);
	std::vector<file_system::FileSystemEntry> id_at_position;

	std::string get_formatted_filename(std::string filename, mode_t mode);

	bool quit;
	std::string cwd;
	std::string default_filename;

	ListWidget files_list;
};

} // namespace newsboat

#endif /* NEWSBOAT_FILEBROWSERFORMACTION_H_ */

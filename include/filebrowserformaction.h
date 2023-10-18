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

	void set_default_filename(const Filepath& fn)
	{
		default_filename = fn.clone();
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
	bool process_operation(Operation op,
		bool automatic = false,
		std::vector<std::string>* args = nullptr) override;
	void update_title(const Filepath& working_directory);

	void add_file(std::vector<file_system::FileSystemEntry>& id_at_position,
		Filepath filename);
	std::string get_filename_suggestion(const std::string& s); // Probably unnecessary method!
	std::vector<file_system::FileSystemEntry> id_at_position;
	std::vector<std::string> lines;

	Filepath get_formatted_filename(Filepath filename, mode_t mode);

	Filepath default_filename;

	ListWidget files_list;
};

} // namespace newsboat

#endif /* NEWSBOAT_FILEBROWSERFORMACTION_H_ */

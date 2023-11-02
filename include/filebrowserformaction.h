#ifndef NEWSBOAT_FILEBROWSERFORMACTION_H_
#define NEWSBOAT_FILEBROWSERFORMACTION_H_

#include <sys/stat.h>
#include <grp.h>

#include "configcontainer.h"
#include "file_system.h"
#include "formaction.h"
#include "listwidget.h"
#include "stflrichtext.h"

namespace newsboat {

class FileBrowserFormAction : public FormAction {
public:
	FileBrowserFormAction(View&, std::string formstr, ConfigContainer* cfg);
	~FileBrowserFormAction() override = default;
	void prepare() override;
	void init() override;
	std::vector<KeyMapHintEntry> get_keymap_hint() const override;

	void set_default_filename(const Filepath& fn)
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
	bool process_operation(Operation op,
		const std::vector<std::string>& args,
		BindingType bindingType = BindingType::BindKey) override;
	void update_title(const Filepath& working_directory);

	void add_file(std::vector<file_system::FileSystemEntry>& id_at_position,
		const Filepath& filename);
	std::vector<file_system::FileSystemEntry> id_at_position;
	std::vector<StflRichText> lines;

	Filepath get_formatted_filename(const Filepath& filename, mode_t mode);

	LineView file_prompt_line;
	Filepath default_filename;

	ListWidget files_list;

	View& view;
};

} // namespace newsboat

#endif /* NEWSBOAT_FILEBROWSERFORMACTION_H_ */

#ifndef NEWSBOAT_DIRBROWSERFORMACTION_H
#define NEWSBOAT_DIRBROWSERFORMACTION_H

#include <sys/stat.h>
#include <grp.h>

#include "configcontainer.h"
#include "filesystembrowser.h"
#include "formaction.h"
#include "listformatter.h"
#include "listwidget.h"

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
		std::vector<FileSystemBrowser::FileSystemEntry>& id_at_position,
		std::string dirname);
	std::vector<FileSystemBrowser::FileSystemEntry> id_at_position;

	std::string get_formatted_dirname(std::string dirname, mode_t mode);

	ListWidget files_list;
};

} // namespace newsboat

#endif //NEWSBOAT_DIRBROWSERFORMACTION_H

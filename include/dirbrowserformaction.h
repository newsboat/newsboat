#ifndef NEWSBOAT_DIRBROWSERFORMACTION_H
#define NEWSBOAT_DIRBROWSERFORMACTION_H

#include <sys/stat.h>
#include <grp.h>

#include "configcontainer.h"
#include "file_system.h"
#include "formaction.h"
#include "listformatter.h"
#include "listwidget.h"
#include "utf8string.h"

namespace newsboat {

class DirBrowserFormAction : public FormAction {
public:
	DirBrowserFormAction(View*, Utf8String formstr, ConfigContainer* cfg);
	~DirBrowserFormAction() override;
	void prepare() override;
	void init() override;
	const std::vector<KeyMapHintEntry>& get_keymap_hint() const override;

	Utf8String id() const override
	{
		return "dirbrowser";
	}
	Utf8String title() override;

private:
	bool process_operation(Operation op,
		bool automatic = false,
		std::vector<Utf8String>* args = nullptr) override;
	void update_title(const Utf8String& working_directory);

	void add_directory(ListFormatter& listfmt,
		std::vector<file_system::FileSystemEntry>& id_at_position,
		Utf8String dirname);
	std::vector<file_system::FileSystemEntry> id_at_position;

	Utf8String get_formatted_dirname(Utf8String dirname, mode_t mode);

	ListWidget files_list;
};

} // namespace newsboat

#endif //NEWSBOAT_DIRBROWSERFORMACTION_H

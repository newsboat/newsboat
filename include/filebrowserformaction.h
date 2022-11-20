#ifndef NEWSBOAT_FILEBROWSERFORMACTION_H_
#define NEWSBOAT_FILEBROWSERFORMACTION_H_

#include <sys/stat.h>
#include <grp.h>

#include "configcontainer.h"
#include "file_system.h"
#include "formaction.h"
#include "listformatter.h"
#include "listwidget.h"
#include "utf8string.h"

namespace newsboat {

class FileBrowserFormAction : public FormAction {
public:
	FileBrowserFormAction(View*, Utf8String formstr, ConfigContainer* cfg);
	~FileBrowserFormAction() override;
	void prepare() override;
	void init() override;
	const std::vector<KeyMapHintEntry>& get_keymap_hint() const override;

	void set_default_filename(const Utf8String& fn)
	{
		default_filename = fn;
	}

	Utf8String id() const override
	{
		return "filebrowser";
	}
	Utf8String title() override;

private:
	bool process_operation(Operation op,
		bool automatic = false,
		std::vector<Utf8String>* args = nullptr) override;
	void update_title(const Utf8String& working_directory);

	void add_file(ListFormatter& listfmt,
		std::vector<file_system::FileSystemEntry>& id_at_position,
		Utf8String filename);
	Utf8String get_filename_suggestion(const Utf8String& s);
	std::vector<file_system::FileSystemEntry> id_at_position;

	Utf8String get_formatted_filename(Utf8String filename, mode_t mode);

	Utf8String default_filename;

	ListWidget files_list;
};

} // namespace newsboat

#endif /* NEWSBOAT_FILEBROWSERFORMACTION_H_ */

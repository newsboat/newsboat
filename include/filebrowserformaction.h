#ifndef NEWSBOAT_FILEBROWSERFORMACTION_H_
#define NEWSBOAT_FILEBROWSERFORMACTION_H_

#include "configcontainer.h"
#include "formaction.h"

namespace newsboat {

class FileBrowserFormAction : public FormAction {
public:
	FileBrowserFormAction(View*, std::string formstr, ConfigContainer* cfg);
	~FileBrowserFormAction() override;
	void prepare() override;
	void init() override;
	KeyMapHintEntry* get_keymap_hint() override;

	void set_dir(const std::string& d)
	{
		dir = d;
	}
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
	void process_operation(Operation op,
		bool automatic = false,
		std::vector<std::string>* args = nullptr) override;

	std::string add_file(std::string filename);
	std::string get_filename_suggestion(const std::string& s);
	std::string get_rwx(unsigned short val);

	char get_filetype(mode_t mode);
	std::string get_owner(uid_t uid);
	std::string get_group(gid_t gid);
	std::string
	get_formatted_filename(std::string filename, char ftype, mode_t mode);

	bool quit;
	std::string cwd;
	std::string dir;
	std::string default_filename;

};

} // namespace newsboat

#endif /* NEWSBOAT_FILEBROWSERFORMACTION_H_ */

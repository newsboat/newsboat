#ifndef NEWSBOAT_FILEBROWSERFORMACTION_H_
#define NEWSBOAT_FILEBROWSERFORMACTION_H_

#include "formaction.h"

namespace newsboat {

class FileBrowserFormaction : public Formaction {
public:
	FileBrowserFormaction(View*, std::string formstr);
	~FileBrowserFormaction() override;
	void prepare() override;
	void init() override;
	keymap_hint_entry* get_keymap_hint() override;

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
	void process_operation(operation op,
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

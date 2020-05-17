#include "dirbrowserformaction.h"

#include <algorithm>
#include <cinttypes>
#include <cstring>
#include <curses.h>
#include <dirent.h>
#include <grp.h>
#include <iomanip>
#include <iostream>
#include <pwd.h>
#include <sstream>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "config.h"
#include "fmtstrformatter.h"
#include "logger.h"
#include "strprintf.h"
#include "utils.h"
#include "view.h"

namespace newsboat {

DirBrowserFormAction::DirBrowserFormAction(View* vv,
	std::string formstr,
	ConfigContainer* cfg)
	: FormAction(vv, formstr, cfg)
	, files_list("files", FormAction::f)
{
	// In DirBrowser, keyboard focus is at the input field, so user can't
	// possibly use 'q' key to exit the dialog
	KeyMap* keys = vv->get_keymap();
	keys->set_key(OP_QUIT, "ESC", id());
	vv->set_keymap(keys);
}

DirBrowserFormAction::~DirBrowserFormAction() {}

char get_filetype(mode_t mode)
{
	static struct FlagChar {
		mode_t flag;
		char ftype;
	} flags[] = {{S_IFREG, '-'},
		{S_IFDIR, 'd'},
		{S_IFBLK, 'b'},
		{S_IFCHR, 'c'},
		{S_IFIFO, 'p'},
		{S_IFLNK, 'l'},
		{S_IFSOCK, 's'},
		{0, 0}
	};

	for (unsigned int i = 0; flags[i].flag != 0; i++) {
		if ((mode & S_IFMT) == flags[i].flag) {
			return flags[i].ftype;
		}
	}
	return '?';
}

bool DirBrowserFormAction::process_operation(Operation op,
	bool /* automatic */,
	std::vector<std::string>* /* args */)
{
	switch (op) {
	case OP_OPEN: {
		/*
		 * whenever "ENTER" is hit, we need to distinguish two different
		 * cases:
		 *   - the focus is in the list of directories, then we need to
		 * set the dirname field to the currently selected entry
		 *   - the focus is in the dirname field, then the dirname
		 * needs to be returned.
		 */
		LOG(Level::DEBUG, "DirBrowserFormAction: 'opening' item");
		std::string focus = f.get_focus();
		if (focus.length() > 0) {
			if (focus == "files") {
				std::string selection = f.get("listposname");
				char filetype = selection[0];
				selection.erase(0, 1);
				std::string filename(selection);
				switch (filetype) {
				case 'd': {
					int status = ::chdir(filename.c_str());
					LOG(Level::DEBUG,
						"DirBrowserFormAction:OP_OPEN: chdir(%s) = %i",
						filename,
						status);
					f.set("files_pos", "0");
					std::string fn = utils::getcwd();
					update_title(fn);

					if (fn.back() != NEWSBEUTER_PATH_SEP) {
						fn.push_back(NEWSBEUTER_PATH_SEP);
					}

					std::string fnstr =
						f.get("filenametext");
					std::string::size_type base =
						fnstr.find_last_of(NEWSBEUTER_PATH_SEP);
					if (base == std::string::npos) {
						fn.append(fnstr);
					} else {
						fn.append(fnstr,
							base + 1,
							std::string::npos);
					}
					f.set("filenametext", fn);
					do_redraw = true;
				}
				break;
				case '-': {
					std::string fn = utils::getcwd();
					if (fn.back() != NEWSBEUTER_PATH_SEP) {
						fn.push_back(NEWSBEUTER_PATH_SEP);
					}
					fn.append(filename);
					f.set("filenametext", fn);
					f.set_focus("filename");
				}
				break;
				default:
					// TODO: show error message
					break;
				}
			} else {
				curs_set(0);
				v->pop_current_formaction();
			}
		}
	}
	break;
	case OP_SWITCH_FOCUS: {
		LOG(Level::DEBUG, "view::dirbrowser: focusing different widget");
		const std::string focus = f.get_focus();
		if (focus == "files") {
			f.set_focus("filename");
		} else {
			f.set_focus("files");
		}
		break;
	}
	case OP_SK_UP:
		if (f.get_focus() == "files") {
			files_list.move_up(cfg->get_configvalue_as_bool("wrap-scroll"));
		} else {
			f.set_focus("files");
		}
		break;
	case OP_SK_DOWN:
		if (f.get_focus() == "files") {
			if (!files_list.move_down(cfg->get_configvalue_as_bool("wrap-scroll"))) {
				f.set_focus("filename");
			}
		}
		break;
	case OP_SK_HOME:
		if (f.get_focus() == "files") {
			files_list.move_to_first();
		} else {
			f.set("filenametext_pos", "0");
		}
		break;
	case OP_SK_END:
		if (f.get_focus() == "files") {
			files_list.move_to_last();
		} else {
			const std::size_t text_length = f.get("filenametext").length();
			f.set("filenametext_pos", std::to_string(text_length));
		}
		break;
	case OP_SK_PGUP:
		if (f.get_focus() == "files") {
			files_list.move_page_up(cfg->get_configvalue_as_bool("wrap-scroll"));
		}
		break;
	case OP_SK_PGDOWN:
		if (f.get_focus() == "files") {
			files_list.move_page_down(cfg->get_configvalue_as_bool("wrap-scroll"));
		}
		break;
	case OP_QUIT:
		LOG(Level::DEBUG, "view::dirbrowser: quitting");
		curs_set(0);
		v->pop_current_formaction();
		f.set("filenametext", "");
		break;
	case OP_HARDQUIT:
		LOG(Level::DEBUG, "view::dirbrowser: hard quitting");
		while (v->formaction_stack_size() > 0) {
			v->pop_current_formaction();
		}
		f.set("filenametext", "");
		break;
	default:
		break;
	}
	return true;
}

void DirBrowserFormAction::update_title(const std::string& working_directory)
{
	const std::string fileswidth = f.get("files:w");
	const unsigned int width = utils::to_u(fileswidth);

	FmtStrFormatter fmt;
	fmt.register_fmt('N', PROGRAM_NAME);
	fmt.register_fmt('V', utils::program_version());
	fmt.register_fmt('f', working_directory);

	const std::string title = fmt.do_format(
			cfg->get_configvalue("dirbrowser-title-format"), width);

	f.set("head", title);
}

std::vector<std::string> get_sorted_dirlist()
{
	std::vector<std::string> ret;

	const std::string cwdtmp = utils::getcwd();

	DIR* dirp = ::opendir(cwdtmp.c_str());
	if (dirp) {
		struct dirent* de = ::readdir(dirp);
		while (de) {
			if (strcmp(de->d_name, ".") != 0 &&
				strcmp(de->d_name, "..") != 0) {
				struct stat sb;
				auto dpath = strprintf::fmt(
						"%s/%s", cwdtmp, de->d_name);
				if (::lstat(dpath.c_str(), &sb) == 0) {
					char ftype = get_filetype(sb.st_mode);
					if (ftype == 'd') {
						ret.push_back(de->d_name);
					}
				}
			}

			de = ::readdir(dirp);
		}
		::closedir(dirp);
	}

	std::sort(ret.begin(), ret.end());

	if (cwdtmp != "/") {
		ret.insert(ret.begin(), "..");
	}

	return ret;
}

void DirBrowserFormAction::prepare()
{
	/*
	 * prepare is always called before an operation is processed,
	 * and if a redraw is necessary, it updates the list of files
	 * in the current directory.
	 */
	if (do_redraw) {
		std::vector<std::string> directories = get_sorted_dirlist();

		ListFormatter listfmt;

		for (std::string directory : directories) {
			add_directory(listfmt, directory);
		}

		files_list.stfl_replace_lines(listfmt.get_lines_count(), listfmt.format_list());
		do_redraw = false;
	}

	std::string focus = f.get_focus();
	if (focus == "files") {
		curs_set(0);
	} else {
		curs_set(1);
	}
}

void DirBrowserFormAction::init()
{
	set_keymap_hints();

	f.set("fileprompt", _("Directory: "));

	if (dir == "") {
		std::string save_path = cfg->get_configvalue("save-path");

		LOG(Level::DEBUG,
			"view::dirbrowser: save-path is '%s'",
			save_path);

		dir = save_path;
	}

	int status = ::chdir(dir.c_str());
	LOG(Level::DEBUG, "view::dirbrowser: chdir(%s) = %i", dir, status);

	const std::string cwdtmp = utils::getcwd();

	f.set("filenametext", dir);

	// Set position to 0 and back to ensure that the text is visible
	f.run(-1);
	f.set("filenametext_pos", std::to_string(dir.length()));

	update_title(cwdtmp);
}

KeyMapHintEntry* DirBrowserFormAction::get_keymap_hint()
{
	static KeyMapHintEntry hints[] = {{OP_QUIT, _("Cancel")},
		{OP_OPEN, _("Save")},
		{OP_NIL, nullptr}
	};

	return hints;
}

void DirBrowserFormAction::add_directory(ListFormatter& listfmt,
	std::string dirname)
{
	struct stat sb;
	if (::lstat(dirname.c_str(), &sb) == 0) {
		char ftype = get_filetype(sb.st_mode);

		std::string rwxbits = get_rwx(sb.st_mode & 0777);
		std::string owner = get_owner(sb.st_uid);
		std::string group = get_group(sb.st_gid);
		std::string formatteddirname =
			get_formatted_dirname(dirname, ftype, sb.st_mode);

		std::string sizestr = strprintf::fmt(
				"%12" PRIi64,
				// `st_size` is `off_t`, which is a signed integer type of
				// unspecified size. We'll have to bet it's no larger than 64
				// bits.
				static_cast<int64_t>(sb.st_size));
		std::string line = strprintf::fmt("%c%s %s %s %s %s",
				ftype,
				rwxbits,
				owner,
				group,
				sizestr,
				formatteddirname);
		std::string id = strprintf::fmt("%c%s", ftype, Stfl::quote(dirname));
		listfmt.add_line(utils::quote_for_stfl(line), id);
	}
}

std::string DirBrowserFormAction::get_formatted_dirname(std::string dirname,
	char ftype,
	mode_t mode)
{
	char suffix = 0;

	switch (ftype) {
	case 'd':
		suffix = '/';
		break;
	case 'l':
		suffix = '@';
		break;
	case 's':
		suffix = '=';
		break;
	case 'p':
		suffix = '|';
		break;
	default:
		if (mode & S_IXUSR) {
			suffix = '*';
		}
	}

	return strprintf::fmt("%s%c", dirname, suffix);
}

std::string DirBrowserFormAction::get_rwx(unsigned short val)
{
	std::string str;
	const char* bitstrs[] = {
		"---", "--x", "-w-", "-wx", "r--", "r-x", "rw-", "rwx"
	};

	for (int i = 0; i < 3; ++i) {
		unsigned char bits = val % 8;
		val /= 8;
		str.insert(0, bitstrs[bits]);
	}
	return str;
}

std::string DirBrowserFormAction::get_owner(uid_t uid)
{
	struct passwd* spw = getpwuid(uid);
	if (spw) {
		std::string owner = spw->pw_name;
		for (int i = owner.length(); i < 8; ++i) {
			owner.append(" ");
		}
		return owner;
	}
	return "????????";
}

std::string DirBrowserFormAction::get_group(gid_t gid)
{
	struct group* sgr = getgrgid(gid);
	if (sgr) {
		std::string group = sgr->gr_name;
		for (int i = group.length(); i < 8; ++i) {
			group.append(" ");
		}
		return group;
	}
	return "????????";
}

std::string DirBrowserFormAction::title()
{
	return strprintf::fmt(_("Save Files - %s"), utils::getcwd());
}

} // namespace newsboat

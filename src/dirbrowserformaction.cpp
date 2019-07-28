#include "dirbrowserformaction.h"

#include <algorithm>
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
{
	// In DirBrowser, keyboard focus is at the input field, so user can't
	// possibly use 'q' key to exit the dialog
	KeyMap* keys = vv->get_keys();
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
		{0, 0}};

	for (unsigned int i = 0; flags[i].flag != 0; i++) {
		if ((mode & S_IFMT) == flags[i].flag)
			return flags[i].ftype;
	}
	return '?';
}

void DirBrowserFormAction::process_operation(Operation op,
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
		std::string focus = f->get_focus();
		if (focus.length() > 0) {
			if (focus == "files") {
				std::string selection = f->get("listposname");
				char filetype = selection[0];
				selection.erase(0, 1);
				std::string filename(selection);
				switch (filetype) {
				case 'd': {
					std::string fileswidth =
						f->get("files:w");
					unsigned int width =
						utils::to_u(fileswidth);

					FmtStrFormatter fmt;
					fmt.register_fmt('N', PROGRAM_NAME);
					fmt.register_fmt(
						'V', utils::program_version());
					fmt.register_fmt('f', filename);
					f->set("head",
						fmt.do_format(
							cfg->get_configvalue(
								"dir"
								"brow"
								"ser-"
								"title-"
								"forma"
								"t"),
							width));
					int status = ::chdir(filename.c_str());
					LOG(Level::DEBUG,
						"DirBrowserFormAction:OP_"
						"OPEN: "
						"chdir(%s) = %i",
						filename,
						status);
					f->set("listpos", "0");
					std::string fn = utils::getcwd();
					fn.append(NEWSBEUTER_PATH_SEP);
					std::string fnstr =
						f->get("filenametext");
					std::string::size_type base =
						fnstr.find_first_of('/');
					if (base == std::string::npos) {
						fn.append(fnstr);
					} else {
						fn.append(fnstr,
							base,
							std::string::npos);
					}
					f->set("filenametext", fn);
					do_redraw = true;
				} break;
				case '-': {
					std::string fn = utils::getcwd();
					fn.append(NEWSBEUTER_PATH_SEP);
					fn.append(filename);
					f->set("filenametext", fn);
					f->set_focus("filename");
				} break;
				default:
					// TODO: show error message
					break;
				}
			} else {
				curs_set(0);
				v->pop_current_formaction();
			}
		}
	} break;
	case OP_QUIT:
		LOG(Level::DEBUG, "view::dirbrowser: quitting");
		curs_set(0);
		v->pop_current_formaction();
		f->set("filenametext", "");
		break;
	case OP_HARDQUIT:
		LOG(Level::DEBUG, "view::dirbrowser: hard quitting");
		while (v->formaction_stack_size() > 0) {
			v->pop_current_formaction();
		}
		f->set("filenametext", "");
		break;
	default:
		break;
	}
}

std::vector<std::string> get_sorted_dirlist()
{
	std::vector<std::string> ret;

	auto cwdtmp = utils::getcwd();

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

		std::string code = "{list";

		for (std::string directory : directories) {
			code.append(add_directory(directory));
		}

		code.append("}");

		f->modify("files", "replace_inner", code);
		do_redraw = false;
	}

	std::string focus = f->get_focus();
	if (focus == "files") {
		curs_set(0);
	} else {
		curs_set(1);
	}
}

void DirBrowserFormAction::init()
{
	set_keymap_hints();

	f->set("fileprompt", _("Directory: "));

	if (dir == "") {
		std::string save_path = cfg->get_configvalue("save-path");

		LOG(Level::DEBUG,
			"view::dirbrowser: save-path is '%s'",
			save_path);

		dir = save_path;
	}

	int status = ::chdir(dir.c_str());
	LOG(Level::DEBUG, "view::dirbrowser: chdir(%s) = %i", dir, status);

	auto cwdtmp = utils::getcwd();

	f->set("filenametext", dir);

	// Set position to 0 and back to ensure that the text is visible
	f->run(-1);
	f->set("filenametext_pos", std::to_string(dir.length()));

	f->set("head",
		strprintf::fmt(_("%s %s - Save Files - %s"),
			PROGRAM_NAME,
			utils::program_version(),
			cwdtmp));
}

KeyMapHintEntry* DirBrowserFormAction::get_keymap_hint()
{
	static KeyMapHintEntry hints[] = {{OP_QUIT, _("Cancel")},
		{OP_OPEN, _("Save")},
		{OP_NIL, nullptr}};

	return hints;
}

std::string DirBrowserFormAction::add_directory(std::string dirname)
{
	std::string retval;
	struct stat sb;
	if (::lstat(dirname.c_str(), &sb) == 0) {
		char ftype = get_filetype(sb.st_mode);

		std::string rwxbits = get_rwx(sb.st_mode & 0777);
		std::string owner = get_owner(sb.st_uid);
		std::string group = get_group(sb.st_gid);
		std::string formatteddirname =
			get_formatted_dirname(dirname, ftype, sb.st_mode);

		std::string sizestr = strprintf::fmt("%12u", sb.st_size);
		std::string line = strprintf::fmt("%c%s %s %s %s %s",
			ftype,
			rwxbits,
			owner,
			group,
			sizestr,
			formatteddirname);
		retval = strprintf::fmt("{listitem[%c%s] text:%s}",
			ftype,
			Stfl::quote(dirname),
			Stfl::quote(line));
	}
	return retval;
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
		if (mode & S_IXUSR)
			suffix = '*';

	}

	return strprintf::fmt("%s%c", dirname, suffix);
}

std::string DirBrowserFormAction::get_rwx(unsigned short val)
{
	std::string str;
	const char* bitstrs[] = {
		"---", "--x", "-w-", "-wx", "r--", "r-x", "rw-", "rwx"};

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

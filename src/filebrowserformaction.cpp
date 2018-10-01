#include "filebrowserformaction.h"

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
#include "formatstring.h"
#include "logger.h"
#include "strprintf.h"
#include "utils.h"
#include "view.h"

namespace newsboat {

FileBrowserFormaction::FileBrowserFormaction(View* vv, std::string formstr)
	: Formaction(vv, formstr)
	, quit(false)
{
	// In filebrowser, keyboard focus is at the input field, so user can't
	// possibly use 'q' key to exit the dialog
	Keymap* keys = vv->get_keys();
	keys->set_key(OP_QUIT, "ESC", id());
	vv->set_keymap(keys);
}

FileBrowserFormaction::~FileBrowserFormaction() {}

void FileBrowserFormaction::process_operation(operation op,
	bool /* automatic */,
	std::vector<std::string>* /* args */)
{
	switch (op) {
	case OP_OPEN: {
		/*
		 * whenever "ENTER" is hit, we need to distinguish two different
		 * cases:
		 *   - the focus is in the list of files, then we need to set
		 * the filename field to the currently selected entry
		 *   - the focus is in the filename field, then the filename
		 * needs to be returned.
		 */
		LOG(Level::DEBUG, "FileBrowserFormaction: 'opening' item");
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
						Utils::to_u(fileswidth);

					FmtStrFormatter fmt;
					fmt.register_fmt('N', PROGRAM_NAME);
					fmt.register_fmt('V', PROGRAM_VERSION);
					fmt.register_fmt('f', filename);
					f->set("head",
						fmt.do_format(
							v->get_cfg()
								->get_configvalue(
									"filebr"
									"o"
									"wser-"
									"title-"
									"Forma"
									"t"),
							width));
					int status = ::chdir(filename.c_str());
					LOG(Level::DEBUG,
						"FileBrowserFormaction:OP_"
						"OPEN: "
						"chdir(%s) = %i",
						filename,
						status);
					f->set("listpos", "0");
					std::string fn = Utils::getcwd();
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
					std::string fn = Utils::getcwd();
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
				bool do_pop = true;
				std::string fn = f->get("filenametext");
				struct stat sbuf;
				/*
				 * this check is very important, as people will
				 * kill us if they accidentaly overwrote their
				 * files with no further warning...
				 */
				if (::stat(fn.c_str(), &sbuf) != -1) {
					f->set_focus("files");
					if (v->confirm(
						    StrPrintf::fmt(
							    _("Do you really "
							      "want to "
							      "overwrite `%s' "
							      "(y:Yes n:No)? "),
							    fn),
						    _("yn")) == *_("n")) {
						do_pop = false;
					}
					f->set_focus("filenametext");
				}
				if (do_pop) {
					curs_set(0);
					v->pop_current_formaction();
				}
			}
		}
	} break;
	case OP_QUIT:
		LOG(Level::DEBUG, "View::filebrowser: quitting");
		curs_set(0);
		v->pop_current_formaction();
		f->set("filenametext", "");
		break;
	case OP_HARDQUIT:
		LOG(Level::DEBUG, "View::filebrowser: hard quitting");
		while (v->Formaction_stack_size() > 0) {
			v->pop_current_formaction();
		}
		f->set("filenametext", "");
		break;
	default:
		break;
	}
}

std::vector<std::string> get_sorted_filelist()
{
	std::vector<std::string> ret;

	auto cwdtmp = Utils::getcwd();

	DIR* dirp = ::opendir(cwdtmp.c_str());
	if (dirp) {
		struct dirent* de = ::readdir(dirp);
		while (de) {
			if (strcmp(de->d_name, ".") != 0 &&
				strcmp(de->d_name, "..") != 0)
				ret.push_back(de->d_name);
			de = ::readdir(dirp);
		}
		::closedir(dirp);
	}

	std::sort(ret.begin(), ret.end());

	if (cwdtmp != "/")
		ret.insert(ret.begin(), "..");

	return ret;
}

void FileBrowserFormaction::prepare()
{
	/*
	 * prepare is always called before an operation is processed,
	 * and if a redraw is necessary, it updates the list of files
	 * in the current directory.
	 */
	if (do_redraw) {
		std::vector<std::string> files = get_sorted_filelist();

		std::string code = "{list";

		for (std::string filename : files) {
			code.append(add_file(filename));
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

void FileBrowserFormaction::init()
{
	set_keymap_hints();

	f->set("fileprompt", _("File: "));

	if (dir == "") {
		std::string save_path =
			v->get_cfg()->get_configvalue("save-path");

		LOG(Level::DEBUG,
			"View::filebrowser: save-path is '%s'",
			save_path);

		dir = save_path;
	}

	int status = ::chdir(dir.c_str());
	LOG(Level::DEBUG, "View::filebrowser: chdir(%s) = %i", dir, status);

	auto cwdtmp = Utils::getcwd();

	f->set("filenametext", default_filename);

	// Set position to 0 and back to ensure that the text is visible
	f->run(-1);
	f->set("filenametext_pos", std::to_string(default_filename.length()));

	f->set("head",
		StrPrintf::fmt(_("%s %s - Save File - %s"),
			PROGRAM_NAME,
			PROGRAM_VERSION,
			cwdtmp));
}

keymap_hint_entry* FileBrowserFormaction::get_keymap_hint()
{
	static keymap_hint_entry hints[] = {{OP_QUIT, _("Cancel")},
		{OP_OPEN, _("Save")},
		{OP_NIL, nullptr}};
	return hints;
}

std::string FileBrowserFormaction::add_file(std::string filename)
{
	std::string retval;
	struct stat sb;
	if (::lstat(filename.c_str(), &sb) == 0) {
		char ftype = get_filetype(sb.st_mode);

		std::string rwxbits = get_rwx(sb.st_mode & 0777);
		std::string owner = get_owner(sb.st_uid);
		std::string group = get_group(sb.st_gid);
		std::string Formattedfilename =
			get_formatted_filename(filename, ftype, sb.st_mode);

		std::string sizestr = StrPrintf::fmt("%12u", sb.st_size);
		std::string line = StrPrintf::fmt("%c%s %s %s %s %s",
			ftype,
			rwxbits,
			owner,
			group,
			sizestr,
			Formattedfilename);
		retval = StrPrintf::fmt("{listitem[%c%s] text:%s}",
			ftype,
			Stfl::quote(filename),
			Stfl::quote(line));
	}
	return retval;
}

std::string FileBrowserFormaction::get_formatted_filename(std::string filename,
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

	return StrPrintf::fmt("%s%c", filename, suffix);
}

std::string FileBrowserFormaction::get_rwx(unsigned short val)
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

char FileBrowserFormaction::get_filetype(mode_t mode)
{
	static struct flag_char {
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

std::string FileBrowserFormaction::get_owner(uid_t uid)
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

std::string FileBrowserFormaction::get_group(gid_t gid)
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

std::string FileBrowserFormaction::title()
{
	return StrPrintf::fmt(_("Save File - %s"), Utils::getcwd());
}

} // namespace newsboat

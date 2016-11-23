#include <filebrowser_formaction.h>
#include <formatstring.h>
#include <logger.h>
#include <config.h>
#include <view.h>
#include <utils.h>
#include <strprintf.h>

#include <iostream>
#include <iomanip>
#include <sstream>
#include <cstring>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <pwd.h>
#include <grp.h>


namespace newsbeuter {

filebrowser_formaction::filebrowser_formaction(view * vv, std::string formstr)
	: formaction(vv,formstr), quit(false)
{
	// In filebrowser, keyboard focus is at the input field, so user can't
	// possibly use 'q' key to exit the dialog
	keymap *keys = vv->get_keys();
	keys->set_key(OP_QUIT, "ESC", id());
	vv->set_keymap(keys);
}

filebrowser_formaction::~filebrowser_formaction() { }

void filebrowser_formaction::process_operation(operation op, bool /* automatic */, std::vector<std::string> * /* args */) {
	switch (op) {
	case OP_OPEN: {
		/*
		 * whenever "ENTER" is hit, we need to distinguish two different cases:
		 *   - the focus is in the list of files, then we need to set the filename field to the currently selected entry
		 *   - the focus is in the filename field, then the filename needs to be returned.
		 */
		LOG(level::DEBUG,"filebrowser_formaction: 'opening' item");
		std::string focus = f->get_focus();
		if (focus.length() > 0) {
			if (focus == "files") {
				std::string selection = f->get("listposname");
				char filetype = selection[0];
				selection.erase(0,1);
				std::string filename(selection);
				switch (filetype) {
				case 'd': {
					std::string fileswidth = f->get("files:w");
					unsigned int width = utils::to_u(fileswidth);

					fmtstr_formatter fmt;
					fmt.register_fmt('N', PROGRAM_NAME);
					fmt.register_fmt('V', PROGRAM_VERSION);
					fmt.register_fmt('f', filename);
					f->set("head", fmt.do_format(v->get_cfg()->get_configvalue("filebrowser-title-format"), width));
					int status = ::chdir(filename.c_str());
					LOG(level::DEBUG,
							"filebrowser_formaction:OP_OPEN: chdir(%s) = %i",
							filename, status);
					f->set("listpos","0");
					std::string fn = utils::getcwd();
					fn.append(NEWSBEUTER_PATH_SEP);
					std::string fnstr = f->get("filenametext");
					std::string::size_type base = fnstr.find_first_of('/');
					if (base == std::string::npos) {
						fn.append(fnstr);
					} else {
						fn.append(fnstr, base, std::string::npos);
					}
					f->set("filenametext",fn);
					do_redraw = true;
				}
				break;
				case '-': {
					std::string fn = utils::getcwd();
					fn.append(NEWSBEUTER_PATH_SEP);
					fn.append(filename);
					f->set("filenametext",fn);
					f->set_focus("filename");
				}
				break;
				default:
					// TODO: show error message
					break;
				}
			} else {
				bool do_pop = true;
				std::string fn = f->get("filenametext");
				struct stat sbuf;
				/*
				 * this check is very important, as people will kill us if they accidentaly overwrote their files
				 * with no further warning...
				 */
				if (::stat(fn.c_str(), &sbuf)!=-1) {
					f->set_focus("files");
					if (v->confirm(strprintf::fmt(_("Do you really want to overwrite `%s' (y:Yes n:No)? "), fn), _("yn")) == *_("n")) {
						do_pop = false;
					}
					f->set_focus("filenametext");
				}
				if (do_pop)
					v->pop_current_formaction();
			}
		}
	}
	break;
	case OP_QUIT:
		LOG(level::DEBUG,"view::filebrowser: quitting");
		v->pop_current_formaction();
		f->set("filenametext", "");
		break;
	case OP_HARDQUIT:
		LOG(level::DEBUG,"view::filebrowser: hard quitting");
		while (v->formaction_stack_size() >0) {
			v->pop_current_formaction();
		}
		f->set("filenametext", "");
		break;
	default:
		break;
	}
}

void filebrowser_formaction::prepare() {
	/*
	 * prepare is always called before an operation is processed,
	 * and if a redraw is necessary, it updates the list of files
	 * in the current directory.
	 */
	if (do_redraw) {
		auto cwdtmp = utils::getcwd();
		std::string code = "{list";

		DIR * dirp = ::opendir(cwdtmp.c_str());
		if (dirp) {
			struct dirent * de = ::readdir(dirp);
			while (de) {
				if (strcmp(de->d_name,".")!=0)
					code.append(add_file(de->d_name));
				de = ::readdir(dirp);
			}
			::closedir(dirp);
		}

		code.append("}");

		f->modify("files", "replace_inner", code);
		do_redraw = false;
	}

}

void filebrowser_formaction::init() {
	set_keymap_hints();

	f->set("fileprompt", _("File: "));

	if (dir == "") {
		std::string save_path = v->get_cfg()->get_configvalue("save-path");

		LOG(level::DEBUG,"view::filebrowser: save-path is '%s'",save_path);

		dir = save_path;
	}

	int status = ::chdir(dir.c_str());
	LOG(level::DEBUG, "view::filebrowser: chdir(%s) = %i", dir, status);

	auto cwdtmp = utils::getcwd();

	f->set("filenametext", default_filename);

	f->set("head", strprintf::fmt(_("%s %s - Save File - %s"), PROGRAM_NAME, PROGRAM_VERSION, cwdtmp));
}

keymap_hint_entry * filebrowser_formaction::get_keymap_hint() {
	static keymap_hint_entry hints[] = {
		{ OP_QUIT, _("Cancel") },
		{ OP_OPEN, _("Save") },
		{ OP_NIL, nullptr }
	};
	return hints;
}

std::string filebrowser_formaction::add_file(std::string filename) {
	std::string retval;
	struct stat sb;
	if (::stat(filename.c_str(),&sb)==0) {
		char ftype = get_filetype(sb.st_mode);

		std::string rwxbits = get_rwx(sb.st_mode & 0777);
		std::string owner = get_owner(sb.st_uid);
		std::string group = get_group(sb.st_gid);

		std::string sizestr = strprintf::fmt("%12u", sb.st_size);
		std::string line = strprintf::fmt("%c%s %s %s %s %s", ftype, rwxbits, owner, group, sizestr, filename);
		retval = strprintf::fmt("{listitem[%c%s] text:%s}", ftype, stfl::quote(filename), stfl::quote(line));
	}
	return retval;
}

std::string filebrowser_formaction::get_rwx(unsigned short val) {
	std::string str;
	const char * bitstrs[] = { "---", "--x", "-w-", "-wx", "r--", "r-x", "rw-", "rwx" };
	for (int i=0; i<3; ++i) {
		unsigned char bits = val % 8;
		val /= 8;
		str.insert(0, bitstrs[bits]);
	}
	return str;
}

char filebrowser_formaction::get_filetype(mode_t mode) {
	static struct flag_char {
		mode_t flag;
		char ftype;
	} flags[] = {
		{ S_IFREG, '-' }, { S_IFDIR, 'd' }, { S_IFBLK, 'b' }, { S_IFCHR, 'c' },
		{ S_IFIFO, 'p' }, { S_IFLNK, 'l' }, { 0      ,  0  }
	};
	for (unsigned int i=0; flags[i].flag != 0; i++) {
		if (mode & flags[i].flag)
			return flags[i].ftype;
	}
	return '?';
}

std::string filebrowser_formaction::get_owner(uid_t uid) {
	struct passwd * spw = getpwuid(uid);
	if (spw) {
		std::string owner = spw->pw_name;
		for (int i=owner.length(); i<8; ++i) {
			owner.append(" ");
		}
		return owner;
	}
	return "????????";
}

std::string filebrowser_formaction::get_group(gid_t gid) {
	struct group * sgr = getgrgid(gid);
	if (sgr) {
		std::string group = sgr->gr_name;
		for (int i=group.length(); i<8; ++i) {
			group.append(" ");
		}
		return group;
	}
	return "????????";
}

std::string filebrowser_formaction::title() {
	return strprintf::fmt(_("Save File - %s"), utils::getcwd());
}

}

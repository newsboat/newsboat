#include <filebrowser_formaction.h>
#include <formatstring.h>
#include <logger.h>
#include <config.h>
#include <view.h>
#include <utils.h>

#include <iostream>
#include <iomanip>
#include <sstream>
#include <cstring>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/param.h>
#include <pwd.h>
#include <grp.h>


namespace newsbeuter {

filebrowser_formaction::filebrowser_formaction(view * vv, std::string formstr) 
	: formaction(vv,formstr), quit(false) { }

filebrowser_formaction::~filebrowser_formaction() { }

void filebrowser_formaction::process_operation(operation op, bool /* automatic */, std::vector<std::string> * /* args */) {
	switch (op) {
		case OP_OPEN: 
			{
				/*
				 * whenever "ENTER" is hit, we need to distinguish two different cases:
				 *   - the focus is in the list of files, then we need to set the filename field to the currently selected entry
				 *   - the focus is in the filename field, then the filename needs to be returned.
				 */
				LOG(LOG_DEBUG,"filebrowser_formaction: 'opening' item");
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
									std::istringstream is(fileswidth);
									unsigned int width;
									is >> width;

									fmtstr_formatter fmt;
									fmt.register_fmt('N', PROGRAM_NAME);
									fmt.register_fmt('V', PROGRAM_VERSION);
									// we use O only to distinguish between "open file" and "save file" for the %? format:
									fmt.register_fmt('O', (type == FBT_OPEN) ? "dummy" : ""); 
									fmt.register_fmt('f', filename);
									f->set("head", fmt.do_format(v->get_cfg()->get_configvalue("filebrowser-title-format"), width));
								}
								::chdir(filename.c_str());
								f->set("listpos","0");
								if (type == FBT_SAVE) {
									char cwdtmp[MAXPATHLEN];
									::getcwd(cwdtmp,sizeof(cwdtmp));
									std::string fn(cwdtmp);
									fn.append(NEWSBEUTER_PATH_SEP);
									std::string fnstr = f->get("filenametext");
									const char * base = strrchr(fnstr.c_str(),'/');
									if (!base)
										base = fnstr.c_str();
									fn.append(base);
									f->set("filenametext",fn);
								}
								do_redraw = true;
								break;
							case '-': 
								{
									char cwdtmp[MAXPATHLEN];
									::getcwd(cwdtmp,sizeof(cwdtmp));
									std::string fn(cwdtmp);
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
						if (::stat(fn.c_str(), &sbuf)!=-1 && type == FBT_SAVE) {
							f->set_focus("files");
							if (v->confirm(utils::strprintf(_("Do you really want to overwrite `%s' (y:Yes n:No)? "), fn.c_str()), _("yn")) == *_("n")) {
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
			LOG(LOG_DEBUG,"view::filebrowser: quitting");
			v->pop_current_formaction();
			f->set("filenametext", "");
			break;
		case OP_HARDQUIT:
			LOG(LOG_DEBUG,"view::filebrowser: hard quitting");
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
		char cwdtmp[MAXPATHLEN];
		std::string code = "{list";
		::getcwd(cwdtmp,sizeof(cwdtmp));
		
		DIR * dirp = ::opendir(cwdtmp);
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
	char cwdtmp[MAXPATHLEN];
	::getcwd(cwdtmp,sizeof(cwdtmp));

	set_keymap_hints();

	f->set("fileprompt", _("File: "));

	if (dir == "") {
		std::string save_path = v->get_cfg()->get_configvalue("save-path");

		LOG(LOG_DEBUG,"view::filebrowser: save-path is '%s'",save_path.c_str());

		dir = save_path;
	}

	LOG(LOG_DEBUG, "view::filebrowser: chdir(%s)", dir.c_str());
			
	::chdir(dir.c_str());
	::getcwd(cwdtmp,sizeof(cwdtmp));
	
	f->set("filenametext", default_filename);
	
	std::string buf;
	if (type == FBT_OPEN) {
		buf = utils::strprintf(_("%s %s - Open File - %s"), PROGRAM_NAME, PROGRAM_VERSION, cwdtmp);
	} else {
		buf = utils::strprintf(_("%s %s - Save File - %s"), PROGRAM_NAME, PROGRAM_VERSION, cwdtmp);
	}
	f->set("head", buf);
}

keymap_hint_entry * filebrowser_formaction::get_keymap_hint() {
	static keymap_hint_entry hints[] = {
		{ OP_QUIT, _("Cancel") },
		{ OP_OPEN, _("Save") },
		{ OP_NIL, NULL }
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

		std::string sizestr = utils::strprintf("%12u", sb.st_size);
		std::string line = utils::strprintf("%c%s %s %s %s %s", ftype, rwxbits.c_str(), owner.c_str(), group.c_str(), sizestr.c_str(), filename.c_str());
		retval = utils::strprintf("{listitem[%c%s] text:%s}", ftype, stfl::quote(filename).c_str(), stfl::quote(line).c_str());
	}
	return retval;
}

std::string filebrowser_formaction::get_rwx(unsigned short val) {
	std::string str;
	const char * bitstrs[] = { "---", "--x", "-w-", "-wx", "r--", "r-x", "rw-", "rwx" };
	for (int i=0;i<3;++i) {
		unsigned char bits = val % 8;
		val /= 8;
		str.insert(0, bitstrs[bits]);
	}
	return str;
}

char filebrowser_formaction::get_filetype(mode_t mode) {
	static struct flag_char {
		mode_t flag; char ftype;
	} flags[] = {
		{ S_IFREG, '-' }, { S_IFDIR, 'd' }, { S_IFBLK, 'b' }, { S_IFCHR, 'c' },
		{ S_IFIFO, 'p' }, { S_IFLNK, 'l' }, { 0      ,  0  }
	};
	for (unsigned int i=0;flags[i].flag != 0;i++) {
		if (mode & flags[i].flag)
			return flags[i].ftype;
	}
	return '?';
}

std::string filebrowser_formaction::get_owner(uid_t uid) {
	struct passwd * spw = getpwuid(uid);
	if (spw) {
		std::string owner = spw->pw_name;
		for (int i=owner.length();i<8;++i) {
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
		for (int i=group.length();i<8;++i) {
			group.append(" ");
		}
		return group;
	}
	return "????????";
}

std::string filebrowser_formaction::title() {
	char cwdtmp[MAXPATHLEN];
	::getcwd(cwdtmp,sizeof(cwdtmp));
	if (type == FBT_OPEN)
		return utils::strprintf(_("Open File - %s"), cwdtmp);
	else
		return utils::strprintf(_("Save File - %s"), cwdtmp);
}

}

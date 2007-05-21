#include <filebrowser_formaction.h>
#include <logger.h>
#include <config.h>
#include <view.h>

#include <iostream>
#include <iomanip>
#include <sstream>

#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/param.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>


namespace newsbeuter {

filebrowser_formaction::filebrowser_formaction(view * vv, std::string formstr) 
	: formaction(vv,formstr), quit(false) { }

filebrowser_formaction::~filebrowser_formaction() { }

void filebrowser_formaction::process_operation(operation op, int /*rawchar*/) {
	char buf[1024];
	switch (op) {
		case OP_OPEN: 
			{
				GetLogger().log(LOG_DEBUG,"filebrowser_formaction: 'opening' item");
				std::string focus = f->get_focus();
				if (focus.length() > 0) {
					if (focus == "files") {
						std::string selection = fancy_unquote(f->get("listposname"));
						char filetype = selection[0];
						selection.erase(0,1);
						std::string filename(selection);
						switch (filetype) {
							case 'd':
								if (type == FBT_OPEN) {
									snprintf(buf, sizeof(buf), _("%s %s - Open File - %s"), PROGRAM_NAME, PROGRAM_VERSION, filename.c_str());
								} else {
									snprintf(buf, sizeof(buf), _("%s %s - Save File - %s"), PROGRAM_NAME, PROGRAM_VERSION, filename.c_str());
								}
								f->set("head", buf);
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
						if (::stat(fn.c_str(), &sbuf)!=-1 && type == FBT_SAVE) {
							char buf[2048];
							snprintf(buf,sizeof(buf), _("Do you really want to overwrite `%s' (y:Yes n:No)? "), fn.c_str());
							f->set_focus("files");
							if (v->confirm(buf, "yn") == 'n') {
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
			GetLogger().log(LOG_DEBUG,"view::filebrowser: quitting");
			v->pop_current_formaction();
			f->set("filenametext", "");
		default:
			break;
	}
}

void filebrowser_formaction::prepare() {
	if (do_redraw) {
		char cwdtmp[MAXPATHLEN];
		std::string code = "{list";
		::getcwd(cwdtmp,sizeof(cwdtmp));
		
		DIR * dir = ::opendir(cwdtmp);
		if (dir) {
			struct dirent * de = ::readdir(dir);
			while (de) {
				if (strcmp(de->d_name,".")!=0)
					code.append(add_file(de->d_name));
				de = ::readdir(dir);
			}
			::closedir(dir);
		}
		
		code.append("}");
		
		// GetLogger().log(LOG_DEBUG, "filebrowser: code = %s", code.c_str());
		f->modify("files", "replace_inner", code);
		do_redraw = false;
	}

}

void filebrowser_formaction::init() {
	char cwdtmp[MAXPATHLEN];
	::getcwd(cwdtmp,sizeof(cwdtmp));
	std::string cwd = cwdtmp;

	f->set("fileprompt", _("File: "));

	if (dir == "") {
		std::string save_path = v->get_cfg()->get_configvalue("save-path");

		GetLogger().log(LOG_DEBUG,"view::filebrowser: save-path is '%s'",save_path.c_str());

		dir = save_path;
	}

	GetLogger().log(LOG_DEBUG, "view::filebrowser: chdir(%s)", dir.c_str());
			
	::chdir(dir.c_str());
	::getcwd(cwdtmp,sizeof(cwdtmp));
	
	f->set("filenametext", default_filename);
	
	char buf[1024];
	if (type == FBT_OPEN) {
		snprintf(buf, sizeof(buf), _("%s %s - Open File - %s"), PROGRAM_NAME, PROGRAM_VERSION, cwdtmp);
	} else {
		snprintf(buf, sizeof(buf), _("%s %s - Save File - %s"), PROGRAM_NAME, PROGRAM_VERSION, cwdtmp);
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

std::string filebrowser_formaction::fancy_quote(const std::string& s) {
	std::string x;
	for (unsigned int i=0;i<s.length();++i) {
		if (s[i] != ' ') {
			x.append(1,s[i]);
		} else {
			x.append(1,'/');
		}	
	}	
	return x;
}

std::string filebrowser_formaction::add_file(std::string filename) {
	std::string retval;
	struct stat sb;
	if (::stat(filename.c_str(),&sb)==0) {
		char type = '?';
		if (sb.st_mode & S_IFREG)
			type = '-';
		else if (sb.st_mode & S_IFDIR)
			type = 'd';
		else if (sb.st_mode & S_IFBLK)
			type = 'b';
		else if (sb.st_mode & S_IFCHR)
			type = 'c';
		else if (sb.st_mode & S_IFIFO)
			type = 'p';
		else if (sb.st_mode & S_IFLNK)
			type = 'l';
			
		std::string rwxbits = get_rwx(sb.st_mode & 0777);
		std::string owner = "????????", group = "????????";
		
		struct passwd * spw = getpwuid(sb.st_uid);
		if (spw) {
			owner = spw->pw_name;
			for (int i=owner.length();i<8;++i) {
				owner.append(" ");	
			}	
		}
		struct group * sgr = getgrgid(sb.st_gid);
		if (sgr) {
			group = sgr->gr_name;
			for (int i=group.length();i<8;++i) {
				group.append(" ");
			}
		}
		
		std::ostringstream os;
		os << std::setw(12) << sb.st_size;
		std::string sizestr = os.str();
		
		std::string line;
		line.append(1,type);
		line.append(rwxbits);
		line.append(" ");
		line.append(owner);
		line.append(" ");
		line.append(group);
		line.append(" ");
		line.append(sizestr);
		line.append(" ");
		line.append(filename);
		
		retval = "{listitem[";
		retval.append(1,type);
		retval.append(fancy_quote(filename));
		retval.append("] text:");
		retval.append(stfl::quote(line));
		retval.append("}");
	}
	return retval;
}

std::string filebrowser_formaction::fancy_unquote(const std::string& s) {
	std::string x;
	for (unsigned int i=0;i<s.length();++i) {
		if (s[i] != '/') {
			x.append(1,s[i]);
		} else {
			x.append(1,' ');
		}	
	}	
	return x;	
}

std::string filebrowser_formaction::get_rwx(unsigned short val) {
	std::string str;
	for (int i=0;i<3;++i) {
		unsigned char bits = val % 8;
		val /= 8;
		switch (bits) {
			case 0:
				str = std::string("---") + str;
				break;
			case 1:
				str = std::string("--x") + str;
				break;
			case 2:
				str = std::string("-w-") + str;
				break;
			case 3:
				str = std::string("-wx") + str;
				break;
			case 4:
				str = std::string("r--") + str;
				break;
			case 5:
				str = std::string("r-x") + str;
				break;
			case 6:
				str = std::string("rw-") + str;
				break;
			case 7:
				str = std::string("rwx") + str;
		}	
	}
	return str;
}

}

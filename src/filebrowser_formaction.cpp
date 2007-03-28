#include <filebrowser_formaction.h>
#include <logger.h>
#include <config.h>
#include <view.h>

#include <iostream>

#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/param.h>


namespace newsbeuter {

filebrowser_formaction::filebrowser_formaction(view * vv, std::string formstr) 
	: formaction(vv,formstr), quit(false) { }

filebrowser_formaction::~filebrowser_formaction() { }

void filebrowser_formaction::process_operation(operation op) {
	char buf[1024];
	switch (op) {
		case OP_OPEN: 
			{
				GetLogger().log(LOG_DEBUG,"filebrowser_formaction: 'opening' item");
				std::string focus = f->get_focus();
				if (focus.length() > 0) {
					if (focus == "files") {
						std::string selection = v->fancy_unquote(f->get("listposname"));
						char filetype = selection[0];
						selection.erase(0,1);
						std::string filename(selection);
						switch (filetype) {
							case 'd':
								if (type == FBT_OPEN) {
									snprintf(buf, sizeof(buf), _("Open File - %s"), filename.c_str());
								} else {
									snprintf(buf, sizeof(buf), _("Save File - %s"), filename.c_str());
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
					code.append(v->add_file(de->d_name));
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

		if (save_path.substr(0,2) == "~/") {
			char * homedir = ::getenv("HOME");
			if (homedir) {
				dir.append(homedir);
				dir.append(NEWSBEUTER_PATH_SEP);
				dir.append(save_path.substr(2,save_path.length()-2));
			} else {
				dir = ".";
			}
		} else {
			dir = save_path;
		}
	}

	GetLogger().log(LOG_DEBUG, "view::filebrowser: chdir(%s)", dir.c_str());
			
	::chdir(dir.c_str());
	::getcwd(cwdtmp,sizeof(cwdtmp));
	
	f->set("filenametext", default_filename);
	
	char buf[1024];
	if (type == FBT_OPEN) {
		snprintf(buf, sizeof(buf), _("Open File - %s"), cwdtmp);
	} else {
		snprintf(buf, sizeof(buf), _("Save File - %s"), cwdtmp);
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

}

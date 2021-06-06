#include "filebrowserformaction.h"

#include <algorithm>
#include <cinttypes>
#include <cstring>
#include <ncurses.h>
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
#include "listformatter.h"
#include "logger.h"
#include "strprintf.h"
#include "utils.h"
#include "view.h"

namespace newsboat {

FileBrowserFormAction::FileBrowserFormAction(View* vv,
	std::string formstr,
	ConfigContainer* cfg)
	: FormAction(vv, formstr, cfg)
	, quit(false)
	, files_list("files", FormAction::f, cfg->get_configvalue_as_int("scrolloff"))
{
	// In filebrowser, keyboard focus is at the input field, so user can't
	// possibly use 'q' key to exit the dialog
	KeyMap* keys = vv->get_keymap();
	keys->set_key(OP_QUIT, "ESC", id());
	vv->set_keymap(keys);
}

FileBrowserFormAction::~FileBrowserFormAction() {}

bool FileBrowserFormAction::process_operation(Operation op,
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
		LOG(Level::DEBUG, "FileBrowserFormAction: 'opening' item");
		const std::string focus = f.get_focus();
		if (focus.length() > 0) {
			if (focus == "files") {
				const auto selected_position = files_list.get_position();
				const auto selection = id_at_position[selected_position];
				switch (selection.filetype) {
				case FileSystemBrowser::FileType::Directory: {
					const int status = ::chdir(selection.name.c_str());
					LOG(Level::DEBUG,
						"FileBrowserFormAction:OP_OPEN: chdir(%s) = %i",
						selection.name,
						status);
					files_list.set_position(0);
					std::string fn = utils::getcwd();
					update_title(fn);

					if (fn.back() != NEWSBEUTER_PATH_SEP) {
						fn.push_back(NEWSBEUTER_PATH_SEP);
					}

					const std::string fnstr =
						f.get("filenametext");
					const std::string::size_type base =
						fnstr.find_last_of(NEWSBEUTER_PATH_SEP);
					if (base == std::string::npos) {
						fn.append(fnstr);
					} else {
						fn.append(fnstr,
							base + 1,
							std::string::npos);
					}
					set_value("filenametext", fn);
					do_redraw = true;
				}
				break;
				case FileSystemBrowser::FileType::RegularFile: {
					std::string fn = utils::getcwd();
					if (fn.back() != NEWSBEUTER_PATH_SEP) {
						fn.push_back(NEWSBEUTER_PATH_SEP);
					}
					fn.append(selection.name);
					set_value("filenametext", fn);
					f.set_focus("filename");
				}
				break;
				default:
					// TODO: show error message
					break;
				}
			} else {
				bool do_pop = true;
				std::string fn = f.get("filenametext");
				struct stat sbuf;
				/*
				 * this check is very important, as people will
				 * kill us if they accidentaly overwrote their
				 * files with no further warning...
				 */
				if (::stat(fn.c_str(), &sbuf) != -1) {
					f.set_focus("files");
					if (v->confirm(
							strprintf::fmt(
								_("Do you really want to overwrite `%s' "
									"(y:Yes n:No)? "),
								fn),
							_("yn")) == *_("n")) {
						do_pop = false;
					}
					f.set_focus("filenametext");
				}
				if (do_pop) {
					curs_set(0);
					v->pop_current_formaction();
				}
			}
		}
	}
	break;
	case OP_SWITCH_FOCUS: {
		LOG(Level::DEBUG, "view::filebrowser: focusing different widget");
		const std::string focus = f.get_focus();
		if (focus == "files") {
			f.set_focus("filename");
		} else {
			f.set_focus("files");
		}
		break;
	}
	case OP_PREV:
	case OP_SK_UP:
		if (f.get_focus() == "files") {
			files_list.move_up(cfg->get_configvalue_as_bool("wrap-scroll"));
		} else {
			f.set_focus("files");
		}
		break;
	case OP_NEXT:
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
			set_value("filenametext_pos", "0");
		}
		break;
	case OP_SK_END:
		if (f.get_focus() == "files") {
			files_list.move_to_last();
		} else {
			const std::size_t text_length = f.get("filenametext").length();
			set_value("filenametext_pos", std::to_string(text_length));
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
		LOG(Level::DEBUG, "view::filebrowser: quitting");
		curs_set(0);
		v->pop_current_formaction();
		set_value("filenametext", "");
		break;
	case OP_HARDQUIT:
		LOG(Level::DEBUG, "view::filebrowser: hard quitting");
		while (v->formaction_stack_size() > 0) {
			v->pop_current_formaction();
		}
		set_value("filenametext", "");
		break;
	default:
		break;
	}
	return true;
}

void FileBrowserFormAction::update_title(const std::string& working_directory)
{
	const unsigned int width = files_list.get_width();

	FmtStrFormatter fmt;
	fmt.register_fmt('N', PROGRAM_NAME);
	fmt.register_fmt('V', utils::program_version());
	fmt.register_fmt('f', working_directory);

	const std::string title = fmt.do_format(
			cfg->get_configvalue("filebrowser-title-format"), width);

	set_value("head", title);
}

std::vector<std::string> get_sorted_filelist()
{
	std::vector<std::string> ret;

	const std::string cwdtmp = utils::getcwd();

	DIR* dirp = ::opendir(cwdtmp.c_str());
	if (dirp) {
		struct dirent* de = ::readdir(dirp);
		while (de) {
			if (strcmp(de->d_name, ".") != 0 &&
				strcmp(de->d_name, "..") != 0) {
				ret.push_back(de->d_name);
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

void FileBrowserFormAction::prepare()
{
	/*
	 * prepare is always called before an operation is processed,
	 * and if a redraw is necessary, it updates the list of files
	 * in the current directory.
	 */
	if (do_redraw) {
		const std::string cwdtmp = utils::getcwd();
		update_title(cwdtmp);

		std::vector<std::string> files = get_sorted_filelist();

		ListFormatter listfmt;

		id_at_position.clear();
		for (std::string filename : files) {
			add_file(listfmt, id_at_position, filename);
		}

		files_list.stfl_replace_lines(listfmt);
		do_redraw = false;
	}

	std::string focus = f.get_focus();
	if (focus == "files") {
		curs_set(0);
	} else {
		curs_set(1);
	}
}

void FileBrowserFormAction::init()
{
	set_keymap_hints();

	set_value("fileprompt", _("File: "));

	const std::string save_path = cfg->get_configvalue("save-path");

	LOG(Level::DEBUG,
		"view::filebrowser: save-path is '%s'",
		save_path);

	const int status = ::chdir(save_path.c_str());
	LOG(Level::DEBUG, "view::filebrowser: chdir(%s) = %i", save_path, status);

	set_value("filenametext", default_filename);

	// Set position to 0 and back to ensure that the text is visible
	draw_form();
	set_value("filenametext_pos", std::to_string(default_filename.length()));
}

KeyMapHintEntry* FileBrowserFormAction::get_keymap_hint()
{
	static KeyMapHintEntry hints[] = {{OP_QUIT, _("Cancel")},
		{OP_OPEN, _("Save")},
		{OP_NIL, nullptr}
	};
	return hints;
}

void FileBrowserFormAction::add_file(
	ListFormatter& listfmt,
	std::vector<FileSystemBrowser::FileSystemEntry>& id_at_position,
	std::string filename)
{
	struct stat sb;
	if (::lstat(filename.c_str(), &sb) == 0) {
		const auto ftype = FileSystemBrowser::mode_to_filetype(sb.st_mode);

		const auto rwxbits = FileSystemBrowser::permissions_string(sb.st_mode);
		const auto owner = FileSystemBrowser::get_user_padded(sb.st_uid);
		const auto group = FileSystemBrowser::get_group_padded(sb.st_gid);
		std::string formattedfilename = get_formatted_filename(filename, sb.st_mode);

		std::string sizestr = strprintf::fmt(
				"%12" PRIi64,
				// `st_size` is `off_t`, which is a signed integer type of
				// unspecified size. We'll have to bet it's no larger than 64
				// bits.
				static_cast<int64_t>(sb.st_size));
		std::string line = strprintf::fmt("%c%s %s %s %s %s",
				FileSystemBrowser::filetype_to_char(ftype),
				rwxbits,
				owner,
				group,
				sizestr,
				formattedfilename);
		listfmt.add_line(utils::quote_for_stfl(line));
		id_at_position.push_back(FileSystemBrowser::FileSystemEntry{ftype, filename});
	}
}

std::string FileBrowserFormAction::get_formatted_filename(std::string filename,
	mode_t mode)
{
	const auto suffix = FileSystemBrowser::mode_suffix(mode);
	if (suffix.has_value()) {
		return strprintf::fmt("%s%c", filename, suffix.value());
	} else {
		return filename;
	}
}

std::string FileBrowserFormAction::title()
{
	return strprintf::fmt(_("Save File - %s"), utils::getcwd());
}

} // namespace newsboat

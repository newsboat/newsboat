#include "filebrowserformaction.h"

#include <algorithm>
#include <cinttypes>
#include <cstdint>
#include <cstring>
#include <ncurses.h>
#include <dirent.h>
#include <grp.h>
#include <pwd.h>
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

FileBrowserFormAction::FileBrowserFormAction(View& vv,
	std::string formstr,
	ConfigContainer* cfg,
	Variant variant)
	: FormAction(vv, formstr, cfg)
	, variant(variant)
	, file_prompt_line(f, "fileprompt")
	, filename_input(f, "filename")
	, files_list("files", FormAction::f, cfg->get_configvalue_as_int("scrolloff"))
	, view(vv)
{
}

bool FileBrowserFormAction::handle_event(const Event& event)
{
	const std::string focus = f.get_focus();
	if (focus == "filename") {
		return filename_input.handle_event(event);
	}
	return false;
}

bool FileBrowserFormAction::process_operation(Operation op,
	const std::vector<std::string>& /* args */,
	BindingType /*bindingType*/)
{
	switch (op) {
	case OP_OPEN: {
		/*
		 * whenever "ENTER" is hit, we need to distinguish two different cases:
		 *   - focus on files list => add name of current entry to path and, in
		 *     case of directory, enter the directory
		 *   - focus on filename field => exit dialog and return filename
		 */
		LOG(Level::DEBUG, "FileBrowserFormAction: 'opening' item");
		const std::string focus = f.get_focus();
		if (focus.length() > 0) {
			if (focus == "files") {
				const auto selected_position = files_list.get_position();
				const auto selection = id_at_position[selected_position];
				switch (selection.filetype) {
				case file_system::FileType::Directory: {
					const int status = ::chdir(selection.name.to_locale_string().c_str());
					LOG(Level::DEBUG,
						"FileBrowserFormAction:OP_OPEN: chdir(%s) = %i",
						selection.name,
						status);
					files_list.set_position(0);
					auto fn = utils::getcwd();
					update_title(fn);

					switch (variant) {
					case Variant::FileSelection: {
						const auto fnstr = Filepath::from_locale_string(filename_input.get_value()).file_name();
						if (fnstr) {
							fn.push(*fnstr);
						}
						filename_input.set_value(fn.to_locale_string());
						break;
					}
					case Variant::DirectorySelection: {
						const auto fn_with_trailing_slash = fn.join(Filepath{});
						filename_input.set_value(fn_with_trailing_slash.to_locale_string());
						break;
					}
					}
					do_redraw = true;
				}
				break;
				case file_system::FileType::RegularFile: {
					const auto filename = utils::getcwd().join(selection.name);
					filename_input.set_value(filename.to_locale_string());
					f.set_focus("filename");
				}
				break;
				default:
					// TODO: show error message
					break;
				}
			} else {
				bool do_pop = true;
				if (variant == Variant::FileSelection) {
					std::string fn = filename_input.get_value();
					struct stat sbuf;
					/*
					 * this check is very important, as people will
					 * kill us if they accidentaly overwrote their
					 * files with no further warning...
					 */
					if (::stat(fn.c_str(), &sbuf) != -1) {
						f.set_focus("files");
						if (v.confirm(
								strprintf::fmt(
									_("Do you really want to overwrite `%s' "
										"(y:Yes n:No)? "),
									fn),
								_("yn")) == *_("n")) {
							do_pop = false;
						}
						f.set_focus("filenametext");
					}
				}
				if (do_pop) {
					curs_set(0);
					v.pop_current_formaction();
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
			if (!files_list.move_down(
					cfg->get_configvalue_as_bool("wrap-scroll"))) {
				f.set_focus("filename");
			}
		}
		break;
	case OP_SK_HOME:
		if (f.get_focus() == "files") {
			files_list.move_to_first();
		}
		break;
	case OP_SK_END:
		if (f.get_focus() == "files") {
			files_list.move_to_last();
		}
		break;
	case OP_SK_PGUP:
		if (f.get_focus() == "files") {
			files_list.move_page_up(
				cfg->get_configvalue_as_bool("wrap-scroll"));
		}
		break;
	case OP_SK_PGDOWN:
		if (f.get_focus() == "files") {
			files_list.move_page_down(
				cfg->get_configvalue_as_bool("wrap-scroll"));
		}
		break;
	case OP_SK_HALF_PAGE_UP:
		if (f.get_focus() == "files") {
			files_list.scroll_halfpage_up(cfg->get_configvalue_as_bool("wrap-scroll"));
		}
		break;
	case OP_SK_HALF_PAGE_DOWN:
		if (f.get_focus() == "files") {
			files_list.scroll_halfpage_down(cfg->get_configvalue_as_bool("wrap-scroll"));
		}
		break;
	case OP_QUIT:
		LOG(Level::DEBUG, "view::filebrowser: quitting");
		curs_set(0);
		v.pop_current_formaction();
		filename_input.set_value("");
		break;
	case OP_HARDQUIT:
		LOG(Level::DEBUG, "view::filebrowser: hard quitting");
		while (v.formaction_stack_size() > 0) {
			v.pop_current_formaction();
		}
		filename_input.set_value("");
		break;
	default:
		report_unhandled_operation(op);
		return false;
	}
	return true;
}

void FileBrowserFormAction::update_title(const Filepath& working_directory)
{
	const unsigned int width = files_list.get_width();

	FmtStrFormatter fmt;
	fmt.register_fmt('N', PROGRAM_NAME);
	fmt.register_fmt('V', utils::program_version());
	fmt.register_fmt('f', working_directory.display());

	std::string title{};
	switch (variant) {
	case Variant::FileSelection:
		title = cfg->get_configvalue("filebrowser-title-format");
		break;
	case Variant::DirectorySelection:
		title = cfg->get_configvalue("dirbrowser-title-format");
		break;
	}
	set_title(fmt.do_format(title, width));
}

std::vector<Filepath> get_sorted_filelist()
{
	std::vector<Filepath> ret;

	const auto cwdtmp = utils::getcwd();

	DIR* dirp = ::opendir(cwdtmp.to_locale_string().c_str());
	if (dirp) {
		struct dirent* de = ::readdir(dirp);
		while (de) {
			if (strcmp(de->d_name, ".") != 0 &&
				strcmp(de->d_name, "..") != 0) {
				ret.push_back(Filepath::from_locale_string(de->d_name));
			}
			de = ::readdir(dirp);
		}
		::closedir(dirp);
	}

	std::sort(ret.begin(), ret.end());

	if (cwdtmp != "/"_path) {
		ret.emplace(ret.begin(), ".."_path);
	}

	return ret;
}

std::vector<Filepath> get_sorted_dirlist()
{
	std::vector<Filepath> ret;

	const auto cwdtmp = utils::getcwd();

	DIR* dirp = ::opendir(cwdtmp.to_locale_string().c_str());
	if (dirp) {
		struct dirent* de = ::readdir(dirp);
		while (de) {
			if (strcmp(de->d_name, ".") != 0 &&
				strcmp(de->d_name, "..") != 0) {
				struct stat sb;
				auto entry = Filepath::from_locale_string(de->d_name);
				const auto dpath = cwdtmp.join(entry);
				if (::lstat(dpath.to_locale_string().c_str(), &sb) == 0) {
					const auto ftype = file_system::mode_to_filetype(sb.st_mode);
					if (ftype == file_system::FileType::Directory) {
						ret.emplace_back(std::move(entry));
					}
				}
			}

			de = ::readdir(dirp);
		}
		::closedir(dirp);
	}

	std::sort(ret.begin(), ret.end());

	if (cwdtmp != "/"_path) {
		ret.emplace(ret.begin(), ".."_path);
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
		update_title(utils::getcwd());

		id_at_position.clear();
		lines.clear();
		switch (variant) {
		case Variant::FileSelection:
			for (auto filename : get_sorted_filelist()) {
				add_file(id_at_position, filename);
			}
			break;
		case Variant::DirectorySelection:
			for (auto directory : get_sorted_dirlist()) {
				add_file(id_at_position, directory);
			}
			break;
		}

		auto render_line = [this](std::uint32_t line, std::uint32_t width) -> StflRichText {
			(void)width;
			return lines[line];
		};

		files_list.invalidate_list_content(lines.size(), render_line);
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
	// In filebrowser, keyboard focus is at the input field, so user will be
	// unable to use alphanumeric keys to confirm or quit the dialog (e.g. they
	// can't quit with the default `q` bind).
	KeyMap* keys = view.get_keymap();
	keys->set_key(OP_OPEN, KeyCombination("ENTER"), id());
	keys->set_key(OP_QUIT, KeyCombination("ESC"), id());
	view.set_keymap(keys);

	set_keymap_hints();

	switch (variant) {
	case Variant::FileSelection:
		file_prompt_line.set_text(_("File: "));
		break;
	case Variant::DirectorySelection:
		file_prompt_line.set_text(_("Directory: "));
		break;
	}

	const auto save_path = cfg->get_configvalue_as_filepath("save-path");

	LOG(Level::DEBUG, "view::filebrowser: save-path is '%s'", save_path);

	const int status = ::chdir(save_path.to_locale_string().c_str());
	LOG(Level::DEBUG, "view::filebrowser: chdir(%s) = %i", save_path, status);

	std::string filenametext{};
	switch (variant) {
	case Variant::FileSelection:
		filenametext = default_filename.to_locale_string();
		break;
	case Variant::DirectorySelection:
		filenametext = save_path.to_locale_string();
		break;
	}

	filename_input.set_value(filenametext);

	// Set position to 0 and back to ensure that the text is visible
	draw_form();
	// TODO: #2326 use length by graphemes
	// See: https://github.com/newsboat/newsboat/pull/2561#discussion_r1357376071
	filename_input.set_position(filenametext.length());
}

std::vector<KeyMapHintEntry> FileBrowserFormAction::get_keymap_hint() const
{
	static const std::vector<KeyMapHintEntry> hints = {{OP_QUIT, _("Cancel")},
		{OP_OPEN, _("Save")}
	};
	return hints;
}

void FileBrowserFormAction::add_file(
	std::vector<file_system::FileSystemEntry>& id_at_position,
	const Filepath& filename)
{
	struct stat sb;
	if (::lstat(filename.to_locale_string().c_str(), &sb) == 0) {
		const auto ftype = file_system::mode_to_filetype(sb.st_mode);

		const auto rwxbits = file_system::permissions_string(sb.st_mode);
		const auto owner = file_system::get_user_padded(sb.st_uid);
		const auto group = file_system::get_group_padded(sb.st_gid);
		std::string formattedfilename = get_formatted_filename(filename, sb.st_mode);

		std::string sizestr = strprintf::fmt(
				"%12" PRIi64,
				// `st_size` is `off_t`, which is a signed integer type of
				// unspecified size. We'll have to bet it's no larger than 64
				// bits.
				static_cast<int64_t>(sb.st_size));
		std::string line = strprintf::fmt("%c%s %s %s %s %s",
				file_system::filetype_to_char(ftype),
				rwxbits,
				owner,
				group,
				sizestr,
				formattedfilename);
		lines.push_back(StflRichText::from_plaintext(line));
		id_at_position.push_back(file_system::FileSystemEntry{ftype, filename});
	}
}

std::string FileBrowserFormAction::get_formatted_filename(const Filepath& filename,
	mode_t mode)
{
	const auto filename_str = filename.display();
	const auto suffix = file_system::mode_suffix(mode);
	if (suffix.has_value()) {
		return strprintf::fmt("%s%c", filename_str, suffix.value());
	} else {
		return filename_str;
	}
}

std::string FileBrowserFormAction::title()
{
	switch (variant) {
	case Variant::FileSelection:
		return strprintf::fmt(_("Save File - %s"), utils::getcwd());
		break;
	case Variant::DirectorySelection:
		return strprintf::fmt(_("Save Files - %s"), utils::getcwd());
		break;
	}
	return "";
}

} // namespace newsboat

#include "dirbrowserformaction.h"

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
#include "keycombination.h"
#include "logger.h"
#include "strprintf.h"
#include "utils.h"
#include "view.h"

namespace newsboat {

DirBrowserFormAction::DirBrowserFormAction(View& vv,
	std::string formstr,
	ConfigContainer* cfg)
	: FormAction(vv, formstr, cfg)
	, file_prompt_line(f, "fileprompt")
	, files_list("files", FormAction::f, cfg->get_configvalue_as_int("scrolloff"))
	, view(vv)
{
}

bool DirBrowserFormAction::process_operation(Operation op,
	const std::vector<std::string>& /* args */,
	BindingType /*bindingType*/)
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
		const std::string focus = f.get_focus();
		if (focus.length() > 0) {
			if (focus == "files") {
				const auto selected_position = files_list.get_position();
				const auto selection = id_at_position[selected_position];
				switch (selection.filetype) {
				case file_system::FileType::Directory: {
					const int status = ::chdir(selection.name.to_locale_string().c_str());
					LOG(Level::DEBUG,
						"DirBrowserFormAction:OP_OPEN: chdir(%s) = %i",
						selection.name,
						status);
					files_list.set_position(0);
					const auto fn = utils::getcwd();
					update_title(fn);

					const auto fn_with_trailing_slash = fn.join(Filepath{});
					set_value("filenametext", fn_with_trailing_slash.to_locale_string());
					do_redraw = true;
				}
				break;
				case file_system::FileType::RegularFile: {
					const auto filename = utils::getcwd().join(selection.name);
					set_value("filenametext", filename.to_locale_string());
					f.set_focus("filename");
				}
				break;
				default:
					// TODO: show error message
					break;
				}
			} else {
				curs_set(0);
				v.pop_current_formaction();
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
		LOG(Level::DEBUG, "view::dirbrowser: quitting");
		curs_set(0);
		v.pop_current_formaction();
		set_value("filenametext", "");
		break;
	case OP_HARDQUIT:
		LOG(Level::DEBUG, "view::dirbrowser: hard quitting");
		while (v.formaction_stack_size() > 0) {
			v.pop_current_formaction();
		}
		set_value("filenametext", "");
		break;
	default:
		report_unhandled_operation(op);
		return false;
	}
	return true;
}

void DirBrowserFormAction::update_title(const Filepath& working_directory)
{
	const unsigned int width = files_list.get_width();

	FmtStrFormatter fmt;
	fmt.register_fmt('N', PROGRAM_NAME);
	fmt.register_fmt('V', utils::program_version());
	fmt.register_fmt('f', working_directory.display());

	set_title(fmt.do_format(
			cfg->get_configvalue("dirbrowser-title-format"), width));
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

	if (cwdtmp != Filepath::from_locale_string("/")) {
		ret.emplace(ret.begin(), Filepath::from_locale_string(".."));
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
		update_title(utils::getcwd());

		id_at_position.clear();
		lines.clear();
		for (auto directory : get_sorted_dirlist()) {
			add_directory(id_at_position, directory);
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

void DirBrowserFormAction::init()
{
	// In dirbrowser, keyboard focus is at the input field, so user will be
	// unable to use alphanumeric keys to confirm or quit the dialog (e.g. they
	// can't quit with the default `q` bind).
	KeyMap* keys = view.get_keymap();
	keys->set_key(OP_OPEN, KeyCombination("ENTER"), id());
	keys->set_key(OP_QUIT, KeyCombination("ESC"), id());
	view.set_keymap(keys);

	set_keymap_hints();

	file_prompt_line.set_text(_("Directory: "));

	const auto save_path = cfg->get_configvalue_as_filepath("save-path");

	LOG(Level::DEBUG, "view::dirbrowser: save-path is '%s'", save_path);

	const int status = ::chdir(save_path.to_locale_string().c_str());
	LOG(Level::DEBUG, "view::dirbrowser: chdir(%s) = %i", save_path, status);

	set_value("filenametext", save_path.to_locale_string());

	// Set position to 0 and back to ensure that the text is visible
	draw_form();
	// TODO: #2326 use length by graphemes
	// See: https://github.com/newsboat/newsboat/pull/2561#discussion_r1357376071
	set_value("filenametext_pos",
		std::to_string(save_path.to_locale_string().length()));
}

std::vector<KeyMapHintEntry> DirBrowserFormAction::get_keymap_hint() const
{
	static const std::vector<KeyMapHintEntry> hints = {{OP_QUIT, _("Cancel")},
		{OP_OPEN, _("Save")}
	};

	return hints;
}

void DirBrowserFormAction::add_directory(
	std::vector<file_system::FileSystemEntry>& id_at_position,
	Filepath dirname)
{
	struct stat sb;
	if (::lstat(dirname.to_locale_string().c_str(), &sb) == 0) {
		const auto ftype = file_system::mode_to_filetype(sb.st_mode);

		const auto rwxbits = file_system::permissions_string(sb.st_mode);
		const auto owner = file_system::get_user_padded(sb.st_uid);
		const auto group = file_system::get_group_padded(sb.st_gid);
		std::string formatteddirname = get_formatted_dirname(dirname, sb.st_mode);

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
				formatteddirname);
		lines.push_back(StflRichText::from_plaintext(line));
		id_at_position.push_back(file_system::FileSystemEntry{ftype, dirname});
	}
}

std::string DirBrowserFormAction::get_formatted_dirname(const Filepath& dirname,
	mode_t mode)
{
	const auto dirname_str = dirname.display();
	const auto suffix = file_system::mode_suffix(mode);
	if (suffix.has_value()) {
		return strprintf::fmt("%s%c", dirname_str, suffix.value());
	} else {
		return dirname_str;
	}
}

std::string DirBrowserFormAction::title()
{
	return strprintf::fmt(_("Save Files - %s"), utils::getcwd());
}

} // namespace newsboat

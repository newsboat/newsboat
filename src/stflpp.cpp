#include "stflpp.h"

#include <cerrno>
#include <langinfo.h>
#include <mutex>
#include <ncurses.h>

#include "exception.h"
#include "logger.h"
#include "utils.h"

namespace newsboat {

/*
 * This is a wrapper around the low-level C functions of STFL.
 * In order to make working with std::string easier, this wrapper
 * was created. This is also useful for logging all stfl-related
 * operations if necessary, and for working around bugs in STFL,
 * especially related to stuff like multithreading fuckups.
 */

Stfl::Form::Form(const std::string& text)
	: f(0)
{
	ipool = stfl_ipool_create(
			utils::translit(nl_langinfo(CODESET), "WCHAR_T").c_str());
	if (!ipool) {
		throw Exception(errno);
	}
	f = stfl_create(stfl_ipool_towc(ipool, text.c_str()));
	if (!f) {
		throw Exception(errno);
	}
}

Stfl::Form::~Form()
{
	if (f) {
		stfl_free(f);
	}
	if (ipool) {
		stfl_ipool_destroy(ipool);
	}
}

std::string Stfl::Form::run(int timeout)
{
	const auto event = stfl_ipool_fromwc(ipool, stfl_run(f, timeout));
	if (event != nullptr) {
		return std::string(event);
	} else {
		return "";
	}
}

void Stfl::Form::draw_form()
{
	run(-1);
}

void Stfl::Form::recalculate_widget_dimensions()
{
	run(-3);
}

std::string Stfl::Form::convert(std::wstring input)
{
	const auto output = stfl_ipool_fromwc(ipool, input.c_str());
	return output == nullptr ? "" : std::string{output};
}

std::string Stfl::Form::key_to_string(wint_t wch)
{
	switch (wch) {
	case '\r':
	case '\n':
		return "ENTER";
	case ' ':
		return "SPACE";
	case '\t':
		return "TAB";
	case 27:
		return "ESC";
	case 127:
		return "BACKSPACE";
	}
	if (wch < 32) {
		return keyname(wch);
	} else {
		std::wstring key{static_cast<wchar_t>(wch)};
		return convert(key);
	}
}

std::string Stfl::Form::function_key_to_string(wint_t wch)
{
	if (wch >= KEY_F(0) && wch <= KEY_F(63)) {
		return strprintf::fmt("F%u", wch - KEY_F0);
	}
	const std::string name = keyname(wch);
	if (name.substr(0, 4) == "KEY_") {
		// Drop "KEY_" prefix from name
		return name.substr(4);
	} else {
		return name;
	}
}

Event Stfl::Form::wait_for_event(int timeout)
{
	wtimeout(stdscr, timeout == 0 ? -1 : timeout);

	wint_t wch{};
	const auto rc = wget_wch(stdscr, &wch);
	LOG(Level::DEBUG, "wait_for_event: wget_wch: rc: %d, wch: %u", rc, wch);
	Event event;
	if (rc == ERR) {
		event = {"TIMEOUT", std::nullopt};
	} else if (rc == KEY_CODE_YES) {
		event = {function_key_to_string(wch), std::nullopt};
	} else {
		std::optional<std::string> printableCharacter{};
		if (iswprint(wch)) {
			std::wstring key{static_cast<wchar_t>(wch)};
			printableCharacter = convert(key);
		}
		event = {key_to_string(wch), printableCharacter};
	}
	LOG(Level::DEBUG, "wait_for_event: event: %s (printable character: %s)", event.name,
		event.printableCharacter.has_value() ? "yes" : "no");
	return event;
}


std::string Stfl::Form::get(const std::string& name)
{
	const char* text = stfl_ipool_fromwc(
			ipool, stfl_get(f, stfl_ipool_towc(ipool, name.c_str())));
	std::string retval;
	if (text) {
		retval = text;
	}
	stfl_ipool_flush(ipool);
	return retval;
}

void Stfl::Form::set(const std::string& name, const std::string& value)
{
	stfl_set(f,
		stfl_ipool_towc(ipool, name.c_str()),
		stfl_ipool_towc(ipool, value.c_str()));
	stfl_ipool_flush(ipool);
}

std::string Stfl::Form::get_focus()
{
	const char* focus = stfl_ipool_fromwc(ipool, stfl_get_focus(f));
	std::string retval;
	if (focus) {
		retval = focus;
	}
	stfl_ipool_flush(ipool);
	return retval;
}

void Stfl::Form::set_focus(const std::string& name)
{
	stfl_set_focus(f, stfl_ipool_towc(ipool, name.c_str()));
	LOG(Level::DEBUG, "Stfl::Form::set_focus: %s", name);
}

void Stfl::Form::modify(const std::string& name,
	const std::string& mode,
	const std::string& text)
{
	const wchar_t* wname, *wmode, *wtext;
	wname = stfl_ipool_towc(ipool, name.c_str());
	wmode = stfl_ipool_towc(ipool, mode.c_str());
	wtext = stfl_ipool_towc(ipool, text.c_str());
	stfl_modify(f, wname, wmode, wtext);
	stfl_ipool_flush(ipool);
}

void Stfl::reset()
{
	stfl_reset();
}

static std::mutex quote_mtx;

std::string Stfl::quote(const std::string& text)
{
	std::lock_guard<std::mutex> lock(quote_mtx);
	stfl_ipool* ipool = stfl_ipool_create(
			utils::translit(nl_langinfo(CODESET), "WCHAR_T").c_str());
	std::string retval = stfl_ipool_fromwc(
			ipool, stfl_quote(stfl_ipool_towc(ipool, text.c_str())));
	stfl_ipool_destroy(ipool);
	return retval;
}

std::string Stfl::Form::dump(const std::string& name, const std::string& prefix,
	int focus)
{
	const char* text = stfl_ipool_fromwc(ipool,
			stfl_dump(f,
				stfl_ipool_towc(ipool, name.c_str()),
				stfl_ipool_towc(ipool, prefix.c_str()),
				focus));
	std::string retval;
	if (text) {
		retval = text;
	}
	stfl_ipool_flush(ipool);
	return retval;
}

} // namespace newsboat

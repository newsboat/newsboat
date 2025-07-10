#include "stflpp.h"

#include <cerrno>
#include <langinfo.h>
#include <mutex>

#include "exception.h"
#include "logger.h"
#include "utils.h"

namespace Newsboat {

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

} // namespace Newsboat

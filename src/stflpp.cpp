#include <stflpp.h>
#include <logger.h>
#include <exception.h>
#include <cerrno>

#include <langinfo.h>

using namespace newsbeuter;

/*
 * This is a wrapper around the low-level C functions of STFL.
 * In order to make working with std::string easier, this wrapper
 * was created. This is also useful for logging all stfl-related
 * operations if necessary, and for working around bugs in STFL,
 * especially related to stuff like multithreading fuckups.
 */

stfl::form::form(const std::string& text) : f(0) {
	ipool = stfl_ipool_create(nl_langinfo(CODESET));
	if (!ipool) {
		throw exception(errno);
	}
	f = stfl_create(stfl_ipool_towc(ipool, text.c_str()));
	if (!f) {
		throw exception(errno);
	}
}

stfl::form::~form() {
	if (f)
		stfl_free(f);
	if (ipool)
		stfl_ipool_destroy(ipool);
}

const char * stfl::form::run(int timeout) {
	return stfl_ipool_fromwc(ipool,stfl_run(f,timeout));
}

std::string stfl::form::get(const std::string& name) {
	const char * text = stfl_ipool_fromwc(ipool,stfl_get(f,stfl_ipool_towc(ipool,name.c_str())));
	std::string retval;
	if (text)
		retval = text;
	stfl_ipool_flush(ipool);
	return retval;
}

void stfl::form::set(const std::string& name, const std::string& value) {
	stfl_set(f, stfl_ipool_towc(ipool,name.c_str()), stfl_ipool_towc(ipool,value.c_str()));
	stfl_ipool_flush(ipool);
}

std::string stfl::form::get_focus() {
	const char * focus = stfl_ipool_fromwc(ipool,stfl_get_focus(f));
	std::string retval;
	if (focus)
		retval = focus;
	stfl_ipool_flush(ipool);
	return retval;
}

void stfl::form::set_focus(const std::string& name) {
	stfl_set_focus(f, stfl_ipool_towc(ipool,name.c_str()));
	GetLogger().log(LOG_DEBUG,"stfl::form::set_focus: %s rc = %d", name.c_str());
	stfl_ipool_flush(ipool);
}

std::string stfl::form::dump(const std::string& name, const std::string& prefix, int focus) {
	const char * text = stfl_ipool_fromwc(ipool,stfl_dump(f, stfl_ipool_towc(ipool,name.c_str()), stfl_ipool_towc(ipool,prefix.c_str()), focus));
	std::string retval;
	if (text)
		retval = text;
	stfl_ipool_flush(ipool);
	return retval;
}

void stfl::form::modify(const std::string& name, const std::string& mode, const std::string& text) {
	GetLogger().log(LOG_INFO, "stfl::form::modify: name = `%s' mode = `%s' text = `%s'", name.c_str(), mode.c_str(), text.c_str());
	const wchar_t * wname, * wmode, * wtext;
	wname = stfl_ipool_towc(ipool,name.c_str());
	wmode = stfl_ipool_towc(ipool,mode.c_str());
	wtext = stfl_ipool_towc(ipool,text.c_str());
	// GetLogger().log(LOG_INFO, "stfl::form::modify: wname = `%ls' mode = `%ls' text = `%ls'", wname, wmode, wtext);
	stfl_modify(f, wname, wmode, wtext);
	stfl_ipool_flush(ipool);
}

std::string stfl::form::lookup(const std::string& path, const std::string& newname) {
	const char * text = stfl_ipool_fromwc(ipool, stfl_lookup(f, stfl_ipool_towc(ipool,path.c_str()), stfl_ipool_towc(ipool,newname.c_str())));
	std::string retval;
	if (text)
		return retval = text;
	stfl_ipool_flush(ipool);
	return retval;
}

std::string stfl::error() {
	stfl_ipool * ipool = stfl_ipool_create(nl_langinfo(CODESET));
	std::string retval = stfl_ipool_fromwc(ipool,stfl_error());
	stfl_ipool_destroy(ipool);
	return retval;
}

void stfl::error_action(const std::string& mode) {
	stfl_ipool * ipool = stfl_ipool_create(nl_langinfo(CODESET));
	stfl_error_action(stfl_ipool_towc(ipool,mode.c_str()));
	stfl_ipool_destroy(ipool);
}

void stfl::reset() {
	stfl_reset();
}

std::string stfl::quote(const std::string& text) {
	static mutex * mtx;
	if (!mtx) {
		mtx = new mutex();
	}
	mtx->lock();
	stfl_ipool * ipool = stfl_ipool_create(nl_langinfo(CODESET));
	GetLogger().log(LOG_DEBUG, "stfl::quote: in: `%s' out: `%ls'", text.c_str(), stfl_quote(stfl_ipool_towc(ipool,text.c_str())));
	std::string retval = stfl_ipool_fromwc(ipool,stfl_quote(stfl_ipool_towc(ipool,text.c_str())));
	stfl_ipool_destroy(ipool);
	mtx->unlock();
	return retval;
}

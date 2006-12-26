#include <stflpp.h>

using namespace noos;

stfl::stfl(const std::string& text) : f(0) {
	f = stfl_create(text.c_str());
	// TODO: exception if f == NULL
}

stfl::~stfl() {
	if (f)
		stfl_free(f);
}

std::string stfl::run(int timeout) {
	const char * event = stfl_run(f,timeout);
	if (event)
		return std::string(event);
	// TODO: exception
	return std::string("");
}

std::string stfl::get(const std::string& name) {
	const char * text = stfl_get(f,name.c_str());
	if (text)
		return std::string(text);
	// TODO: exception
	return std::string("");
}

void stfl::set(const std::string& name, const std::string& value) {
	stfl_set(f, name.c_str(), value.c_str());
}

std::string stfl::get_focus() {
	const char * focus = stfl_get_focus(f);
	if (focus)
		return std::string(focus);
	// TODO: exception
	return std::string("");
}

void stfl::set_focus(const std::string& name) {
	stfl_set_focus(f, name.c_str());
}

std::string stfl::dump(const std::string& name, const std::string& prefix, int focus) {
	const char * text = stfl_dump(f, name.c_str(), prefix.c_str(), focus);
	if (text)
		return std::string(text);
	// TODO: exception
	return std::string("");
}

void stfl::modify(const std::string& name, const std::string& mode, const std::string& text) {
	stfl_modify(f, name.c_str(), mode.c_str(), text.c_str());
}

std::string stfl::lookup(const std::string& path, const std::string& newname) {
	const char * text = stfl_lookup(f, path.c_str(), newname.c_str());
	if (text)
		return std::string(text);
	// TODO: exception
	return std::string("");
}

std::string stfl::error() {
	return std::string(stfl_error());
}

void stfl::error_action(const std::string& mode) {
	stfl_error_action(mode.c_str());
}

void stfl::reset() {
	stfl_reset();
}

std::string stfl::quote(const std::string& text) {
	return stfl_quote(text.c_str());
}

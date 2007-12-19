#include <interpreter.h>
#include <logger.h>
#include <utils.h>

#include <ruby.h>

#include <config.h>

extern "C" void Init_Newsbeuter(); // from libext

namespace newsbeuter {

script_interpreter::script_interpreter() : v(0) {
	GetLogger().log(LOG_DEBUG, "script_interpreter::script_interpreter: initializing Ruby interpreter");
	ruby_init();
	ruby_init_loadpath();

	Init_Newsbeuter();
}

script_interpreter::~script_interpreter() {
	ruby_finalize();
}

void script_interpreter::load_script(const std::string& path) {
	GetLogger().log(LOG_DEBUG, "script_interpreter::load_script: loading file `%s'", path.c_str());
	// VALUE v = rb_require(path.c_str());
	int state;
	rb_load_protect(rb_str_new2(path.c_str()), 0, &state);
	if (state) {
		VALUE c = rb_funcall(rb_gv_get("$!"), rb_intern("to_s"), 0);
		GetLogger().log(LOG_ERROR, "script_interpreter::load_script: %s", RSTRING(c)->ptr);
	}
	// ruby_run();
	// GetLogger().log(LOG_DEBUG, "script_interpreter::load_script: rb_require return value: %s", v == Qtrue ? "Qtrue" : "Qfalse");
}

void script_interpreter::run_function(const std::string& name) {
	int state;
	rb_eval_string_protect(name.c_str(), &state);
	if (state) {
		VALUE c = rb_funcall(rb_gv_get("$!"), rb_intern("to_s"), 0);
		GetLogger().log(LOG_ERROR, "script_interpreter::load_script: %s", RSTRING(c)->ptr);
		char buf[1024];
		snprintf(buf, sizeof(buf), "Error while calling `%s': %s", name.c_str(), RSTRING(c)->ptr);
		v->show_error(buf);
	}
}

action_handler_status script_interpreter::handle_action(const std::string& action, const std::vector<std::string>& params) {
	if (action == "load") {
		if (params.size() < 1)
			return AHS_TOO_FEW_PARAMS;

		for (std::vector<std::string>::const_iterator it=params.begin();it!=params.end();it++) {
			std::string filename = utils::resolve_tilde(*it);
			GetLogger().log(LOG_DEBUG, "script_interpreter::handle_action: loading file %s...", filename.c_str());
			this->load_script(filename);
		}

		return AHS_OK;
	}
	return AHS_INVALID_COMMAND;
}

void script_interpreter::set_view(view * vv) {
	int state;

	v = vv;
	rv = rb_eval_string_protect("Newsbeuter::View.new($ctrl)", &state);
	if (state == 0 && rv != Qnil) {
		DATA_PTR(rv) = v;
		RDATA(rv)->dfree = NULL; // since we didn't allocate the pointer, we don't want to have it freed
		rb_define_variable("view", &rv);
	} else {
		VALUE c = rb_funcall(rb_gv_get("$!"), rb_intern("to_s"), 0);
		GetLogger().log(LOG_ERROR, "script_interpreter::set_view: %s", RSTRING(c)->ptr);
	}
}

void script_interpreter::set_controller(controller * cc) {
	int state;

	c = cc;
	rc = rb_eval_string_protect("Newsbeuter::Controller.new", &state);
	if (state == 0 && rc != Qnil) {
		DATA_PTR(rc) = c;
		RDATA(rc)->dfree = NULL; // see above
		rb_define_variable("ctrl", &rc);
	} else {
		VALUE c = rb_funcall(rb_gv_get("$!"), rb_intern("to_s"), 0);
		GetLogger().log(LOG_ERROR, "script_interpreter::set_controller: %s", RSTRING(c)->ptr);
	}
}

static script_interpreter interp;

script_interpreter * GetInterpreter() {
	return &interp;
}

}

#include <interpreter.h>
#include <logger.h>
#include <utils.h>

#if EMBED_LUA

extern "C" {
#include <lauxlib.h>
#include <lualib.h>
}

namespace newsbeuter {


static int lua_nb_log(lua_State * L);

// view class functions
static int view_cur_form(lua_State * L);
static int view_msg(lua_State * L);

static const struct luaL_reg view_f[] = {
	{ "cur_form", view_cur_form },
	{ "msg", view_msg },
	{ NULL, NULL }
};

// conf class functions
static int conf_setvar(lua_State * L);
static int conf_getvar(lua_State * L);

static const struct luaL_reg conf_f[] = {
	{ "set", conf_setvar },
	{ "get", conf_getvar },
	{ NULL, NULL }
};

script_interpreter::script_interpreter() : L(0), v(0), c(0) {
	L = lua_open();
	GetLogger().log(LOG_DEBUG, "script_interpreter::script_interpreter: L = %p", L);

	luaL_openlibs(L);

	lua_pushcfunction(L, lua_nb_log);
	lua_setglobal(L, "nb_log");

	// register view
	
	luaL_newmetatable(L, "Newsbeuter.view");
	lua_pushstring(L, "__index");
	lua_pushvalue(L, -2);
	lua_settable(L, -3);
	luaL_openlib(L, "view", view_f, 0);

	// register conf
	luaL_newmetatable(L, "Newsbeuter.conf");
	lua_pushstring(L, "__index");
	lua_pushvalue(L, -2);
	lua_settable(L, -3);
	luaL_openlib(L, "conf", conf_f, 0);


}

script_interpreter::~script_interpreter() {
	lua_close(L);
	GetLogger().log(LOG_DEBUG, "script_interpreter::~script_interpreter: closed lua_State handle");
}

void script_interpreter::load_script(const std::string& path) {
	GetLogger().log(LOG_DEBUG, "script_interpreter::load_script: running file `%s'", path.c_str());
	luaL_dofile(L, path.c_str());
}

void script_interpreter::run_function(const std::string& name) {
	lua_getglobal(L, name.c_str());
	if (lua_pcall(L, 0, 0, 0) != 0) {
		GetLogger().log(LOG_DEBUG, "script_interpreter::run_function: error: %s", lua_tostring(L, -1));
		// TODO: throw exception with error from lua_tostring(L, -1)
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

static script_interpreter interp;

script_interpreter * GetInterpreter() {
	return &interp;
}

static int lua_nb_log(lua_State * L) {
	const char * str = luaL_checkstring(L, 1);
	if (str) {
		GetLogger().log(LOG_INFO, "USER-LOG-MSG: %s", str);
	}
	return 0;
}

static int view_cur_form(lua_State * L) {
	lua_pushstring(L, GetInterpreter()->get_view()->id().c_str());
	return 1;
}

static int view_msg(lua_State * L) {
	const char * str = luaL_checkstring(L, 1);
	if (str) {
		GetInterpreter()->get_view()->set_status(str);
	}
	return 0;
}

static int conf_setvar(lua_State * L) {
	const char * key = luaL_checkstring(L, 1);
	const char * value = luaL_checkstring(L, 2);
	if (key && value) {
		GetInterpreter()->get_controller()->get_cfg()->set_configvalue(key, value);
	}
	return 0;
}

static int conf_getvar(lua_State * L) {
	const char * key = luaL_checkstring(L, 1);
	if (key) {
		lua_pushstring(L, GetInterpreter()->get_controller()->get_cfg()->get_configvalue(key).c_str());
	} else {
		lua_pushstring(L, "");
	}
	return 1;
}


}

#endif /* EMBED_LUA */

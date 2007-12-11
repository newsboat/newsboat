#include <interpreter.h>
#include <logger.h>

#if EMBED_LUA

extern "C" {
#include <lauxlib.h>
#include <lualib.h>
}

namespace newsbeuter {

static int lua_nb_log(lua_State * L);

script_interpreter::script_interpreter() : L(0), v(0), c(0) {
	L = lua_open();
	GetLogger().log(LOG_DEBUG, "script_interpreter::script_interpreter: L = %p", L);

	luaL_openlibs(L);

	lua_pushcfunction(L, lua_nb_log);
	lua_setglobal(L, "nb_log");
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


}

#endif /* EMBED_LUA */

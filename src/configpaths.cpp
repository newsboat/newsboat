#include "configpaths.h"

#include <cstring>
#include <fstream>
#include <iostream>
#include <pwd.h>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "config.h"
#include "globals.h"
#include "ruststring.h"
#include "strprintf.h"

extern "C" {
	void* create_rs_configpaths();

	void destroy_rs_configpaths(void* rs_configpaths);

	bool rs_configpaths_initialized(void* rs_configpaths);

	char* rs_configpaths_error_message(void* rs_configpaths);

	void rs_configpaths_process_args(void* rs_configpaths, void* rs_cliargsparser);

	bool rs_configpaths_try_migrate_from_newsbeuter(void* rs_configpaths);

	bool rs_configpaths_create_dirs(void* rs_configpaths);

	char* rs_configpaths_url_file(void* rs_configpaths);

	char* rs_configpaths_cache_file(void* rs_configpaths);

	void rs_configpaths_set_cache_file(void* rs_configpaths, const char*);

	char* rs_configpaths_config_file(void* rs_configpaths);

	char* rs_configpaths_lock_file(void* rs_configpaths);

	char* rs_configpaths_queue_file(void* rs_configpaths);

	char* rs_configpaths_search_file(void* rs_configpaths);

	char* rs_configpaths_cmdline_file(void* rs_configpaths);
}

#define SIMPLY_RUN(NAME) \
	if (rs_configpaths) { \
		rs_configpaths_ ## NAME (rs_configpaths); \
	}

#define GET_VALUE(NAME, DEFAULT) \
	if (rs_configpaths) { \
		return rs_configpaths_ ## NAME (rs_configpaths); \
	} else { \
		return DEFAULT; \
	}

#define GET_STRING(NAME) \
	if (rs_configpaths) { \
		return RustString(rs_configpaths_ ## NAME (rs_configpaths)); \
	} else { \
		return {}; \
	}

namespace newsboat {

ConfigPaths::ConfigPaths()
{
	rs_configpaths = create_rs_configpaths();
}

ConfigPaths::~ConfigPaths()
{
	if (rs_configpaths) {
		destroy_rs_configpaths(rs_configpaths);
	}
}

bool ConfigPaths::initialized() const
{
	GET_VALUE(initialized, false);
}

std::string ConfigPaths::error_message() const
{
	GET_STRING(error_message);
}

void ConfigPaths::process_args(const CliArgsParser& args)
{
	if (rs_configpaths) {
		rs_configpaths_process_args(rs_configpaths, args.get_rust_pointer());
	}
}

bool ConfigPaths::try_migrate_from_newsbeuter()
{
	GET_VALUE(try_migrate_from_newsbeuter, false);
}

bool ConfigPaths::create_dirs() const
{
	GET_VALUE(create_dirs, false);
}

void ConfigPaths::set_cache_file(const std::string& new_cachefile)
{
	if (rs_configpaths) {
		rs_configpaths_set_cache_file(rs_configpaths, new_cachefile.c_str());
	}
}

std::string ConfigPaths::url_file() const
{
	GET_STRING(url_file);
}

std::string ConfigPaths::cache_file() const
{
	GET_STRING(cache_file);
}

std::string ConfigPaths::config_file() const
{
	GET_STRING(config_file);
}

std::string ConfigPaths::lock_file() const
{
	GET_STRING(lock_file);
}

std::string ConfigPaths::queue_file() const
{
	GET_STRING(queue_file);
}

std::string ConfigPaths::search_file() const
{
	GET_STRING(search_file);
}

std::string ConfigPaths::cmdline_file() const
{
	GET_STRING(cmdline_file);
}

} // namespace newsboat

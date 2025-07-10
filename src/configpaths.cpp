#include "configpaths.h"

namespace Newsboat {

ConfigPaths::ConfigPaths()
	: rs_object(configpaths::bridged::create())
{
}

bool ConfigPaths::initialized() const
{
	return Newsboat::configpaths::bridged::initialized(*rs_object);
}

std::string ConfigPaths::error_message() const
{
	return std::string(Newsboat::configpaths::bridged::error_message(*rs_object));
}

void ConfigPaths::process_args(const CliArgsParser& args)
{
	Newsboat::configpaths::bridged::process_args(*rs_object, args.get_rust_ref());
}

bool ConfigPaths::try_migrate_from_newsbeuter()
{
	return Newsboat::configpaths::bridged::try_migrate_from_newsbeuter(*rs_object);
}

bool ConfigPaths::create_dirs() const
{
	return Newsboat::configpaths::bridged::create_dirs(*rs_object);
}

void ConfigPaths::set_cache_file(const std::string& new_cachefile)
{
	Newsboat::configpaths::bridged::set_cache_file(*rs_object, new_cachefile);
}

std::string ConfigPaths::url_file() const
{
	return std::string(Newsboat::configpaths::bridged::url_file(*rs_object));
}

std::string ConfigPaths::cache_file() const
{
	return std::string(Newsboat::configpaths::bridged::cache_file(*rs_object));
}

std::string ConfigPaths::config_file() const
{
	return std::string(Newsboat::configpaths::bridged::config_file(*rs_object));
}

std::string ConfigPaths::lock_file() const
{
	return std::string(Newsboat::configpaths::bridged::lock_file(*rs_object));
}

std::string ConfigPaths::queue_file() const
{
	return std::string(Newsboat::configpaths::bridged::queue_file(*rs_object));
}

std::string ConfigPaths::search_history_file() const
{
	return std::string(Newsboat::configpaths::bridged::search_history_file(*rs_object));
}

std::string ConfigPaths::cmdline_history_file() const
{
	return std::string(Newsboat::configpaths::bridged::cmdline_history_file(*rs_object));
}

} // namespace Newsboat

#include "configpaths.h"

namespace newsboat {

ConfigPaths::ConfigPaths()
	: rs_object(configpaths::bridged::create())
{
}

bool ConfigPaths::initialized() const
{
	return newsboat::configpaths::bridged::initialized(*rs_object);
}

std::string ConfigPaths::error_message() const
{
	return std::string(newsboat::configpaths::bridged::error_message(*rs_object));
}

void ConfigPaths::process_args(const CliArgsParser& args)
{
	newsboat::configpaths::bridged::process_args(*rs_object, args.get_rust_ref());
}

bool ConfigPaths::try_migrate_from_newsbeuter()
{
	return newsboat::configpaths::bridged::try_migrate_from_newsbeuter(*rs_object);
}

bool ConfigPaths::create_dirs() const
{
	return newsboat::configpaths::bridged::create_dirs(*rs_object);
}

void ConfigPaths::set_cache_file(const std::string& new_cachefile)
{
	newsboat::configpaths::bridged::set_cache_file(*rs_object, new_cachefile);
}

Filepath ConfigPaths::url_file() const
{
	return std::string(newsboat::configpaths::bridged::url_file(*rs_object));
}

Filepath ConfigPaths::cache_file() const
{
	return std::string(newsboat::configpaths::bridged::cache_file(*rs_object));
}

Filepath ConfigPaths::config_file() const
{
	return std::string(newsboat::configpaths::bridged::config_file(*rs_object));
}

Filepath ConfigPaths::lock_file() const
{
	return std::string(newsboat::configpaths::bridged::lock_file(*rs_object));
}

Filepath ConfigPaths::queue_file() const
{
	return std::string(newsboat::configpaths::bridged::queue_file(*rs_object));
}

Filepath ConfigPaths::search_history_file() const
{
	return std::string(newsboat::configpaths::bridged::search_history_file(*rs_object));
}

Filepath ConfigPaths::cmdline_history_file() const
{
	return std::string(newsboat::configpaths::bridged::cmdline_history_file(*rs_object));
}

} // namespace newsboat

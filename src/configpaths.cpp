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
	newsboat::configpaths::bridged::process_args(*rs_object,
		*(newsboat::configpaths::bridged::FfiCliArgsParser*)args.get_rust_pointer());
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

std::string ConfigPaths::url_file() const
{
	return std::string(newsboat::configpaths::bridged::url_file(*rs_object));
}

std::string ConfigPaths::cache_file() const
{
	return std::string(newsboat::configpaths::bridged::cache_file(*rs_object));
}

std::string ConfigPaths::config_file() const
{
	return std::string(newsboat::configpaths::bridged::config_file(*rs_object));
}

std::string ConfigPaths::lock_file() const
{
	return std::string(newsboat::configpaths::bridged::lock_file(*rs_object));
}

std::string ConfigPaths::queue_file() const
{
	return std::string(newsboat::configpaths::bridged::queue_file(*rs_object));
}

std::string ConfigPaths::search_file() const
{
	return std::string(newsboat::configpaths::bridged::search_file(*rs_object));
}

std::string ConfigPaths::cmdline_file() const
{
	return std::string(newsboat::configpaths::bridged::cmdline_file(*rs_object));
}

} // namespace newsboat

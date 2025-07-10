#include "history.h"

namespace Newsboat {

History::History()
	: rs_object(history::bridged::create())
{
}

void History::add_line(const std::string& line)
{
	history::bridged::add_line(*rs_object, line);
}

std::string History::previous_line()
{
	return std::string(history::bridged::previous_line(*rs_object));
}

std::string History::next_line()
{
	return std::string(history::bridged::next_line(*rs_object));
}

void History::load_from_file(const std::string& file)
{
	history::bridged::load_from_file(*rs_object, file);
}

void History::save_to_file(const std::string& file, unsigned int limit)
{
	history::bridged::save_to_file(*rs_object, file, limit);
}

} // namespace Newsboat

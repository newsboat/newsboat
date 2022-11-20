#include "history.h"

namespace newsboat {

History::History()
	: rs_object(history::bridged::create())
{
}

void History::add_line(const Utf8String& line)
{
	history::bridged::add_line(*rs_object, line);
}

Utf8String History::previous_line()
{
	return Utf8String(history::bridged::previous_line(*rs_object));
}

Utf8String History::next_line()
{
	return Utf8String(history::bridged::next_line(*rs_object));
}

void History::load_from_file(const Utf8String& file)
{
	history::bridged::load_from_file(*rs_object, file);
}

void History::save_to_file(const Utf8String& file, unsigned int limit)
{
	history::bridged::save_to_file(*rs_object, file, limit);
}

} // namespace newsboat

#include "history.h"

#include <fstream>

#include "ruststring.h"

extern "C" {
	void* rs_history_new();

	void rs_history_free(void* hst);

	void rs_history_add_line(
		void* hst,
		const char* line);

	char* rs_history_previous_line(void* hst);

	char* rs_history_next_line(void* hst);

	void rs_history_load_from_file(
		void* hst,
		const char* file);

	void rs_history_save_to_file(
		void* fmt,
		const char* file,
		unsigned int limit);
}

namespace newsboat {

History::History()
{
	rs_hst = rs_history_new();
}

History::~History()
{
	rs_history_free(rs_hst);
}

void History::add_line(const std::string& line)
{
	rs_history_add_line(rs_hst, line.c_str());
}

std::string History::previous_line()
{
	return RustString(rs_history_previous_line(rs_hst));
}

std::string History::next_line()
{
	return RustString(rs_history_next_line(rs_hst));
}

void History::load_from_file(const std::string& file)
{
	rs_history_load_from_file(rs_hst, file.c_str());
}

void History::save_to_file(const std::string& file, unsigned int limit)
{
	rs_history_save_to_file(rs_hst, file.c_str(), limit);
}

} // namespace newsboat

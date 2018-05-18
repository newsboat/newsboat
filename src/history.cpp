#include "history.h"

#include <fstream>

namespace newsboat {

history::history()
	: idx(0)
{
}

history::~history() {}

void history::add_line(const std::string& line)
{
	/*
	 * When a line is added, we need to do so and
	 * reset the index so that the next prev/next
	 * operations start from the beginning again.
	 */
	if (line.length() > 0) {
		lines.insert(lines.begin(), line);
	}
	idx = 0;
}

std::string history::prev()
{
	if (idx < lines.size()) {
		return lines[idx++];
	}
	if (lines.size() == 0) {
		return "";
	}
	return lines[idx - 1];
}

std::string history::next()
{
	if (idx > 0) {
		return lines[--idx];
	}
	return "";
}

void history::load_from_file(const std::string& file)
{
	std::fstream f;
	f.open(file.c_str(), std::fstream::in);
	if (f.is_open()) {
		std::string line;
		do {
			std::getline(f, line);
			if (!f.eof() && line.length() > 0) {
				add_line(line);
			}
		} while (!f.eof());
	}
}

void history::save_to_file(const std::string& file, unsigned int limit)
{
	std::fstream f;
	f.open(file.c_str(), std::fstream::out | std::fstream::trunc);
	if (f.is_open()) {
		if (limit > lines.size())
			limit = lines.size();
		if (limit > 0) {
			for (unsigned int i = limit - 1; i > 0; i--) {
				f << lines[i] << std::endl;
			}
			f << lines[0] << std::endl;
		}
	}
}

} // namespace newsboat

#include <history.h>

namespace newsbeuter {

history::history() : it(lines.begin()) { }

history::~history() { }

void history::add_line(const std::string& line) {
	if (line.length() > 0) {
		lines.push_front(line);
	}
}

std::string history::prev() {
	if (lines.size() == 0) {
		return "";
	}
	std::string retval;
	if (it == lines.end()) {
		it--;
	}
	retval = *it;
	it++;
	return retval;
}

std::string history::next() {
	if (lines.size() == 0 || it == lines.begin()) {
		return "";
	}
	std::string retval;
	it--;
	retval = *it;
	return retval;
}


}

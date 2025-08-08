#ifndef NEWSBOAT_HISTORY_H_
#define NEWSBOAT_HISTORY_H_

#include "libnewsboat-ffi/src/history.rs.h" // IWYU pragma: export

#include <string>

#include "filepath.h"

namespace newsboat {

class History {
public:
	History();
	~History() = default;
	void add_line(const std::string& line);
	std::string previous_line();
	std::string next_line();
	void load_from_file(const Filepath& file);
	void save_to_file(const Filepath& file, unsigned int limit);

private:
	rust::Box<history::bridged::History> rs_object;
};

} // namespace newsboat

#endif /* NEWSBOAT_HISTORY_H_ */

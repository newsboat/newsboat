#ifndef NEWSBOAT_HISTORY_H_
#define NEWSBOAT_HISTORY_H_

#include "libnewsboat-ffi/src/history.rs.h"

#include "utf8string.h"

namespace newsboat {

class History {
public:
	History();
	~History() = default;
	void add_line(const Utf8String& line);
	Utf8String previous_line();
	Utf8String next_line();
	void load_from_file(const Utf8String& file);
	void save_to_file(const Utf8String& file, unsigned int limit);

private:
	rust::Box<history::bridged::History> rs_object;
};

} // namespace newsboat

#endif /* NEWSBOAT_HISTORY_H_ */

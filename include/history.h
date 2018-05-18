#ifndef NEWSBOAT_HISTORY_H_
#define NEWSBOAT_HISTORY_H_

#include <string>
#include <vector>

namespace newsboat {

class history {
public:
	history();
	~history();
	void add_line(const std::string& line);
	std::string prev();
	std::string next();
	void load_from_file(const std::string& file);
	void save_to_file(const std::string& file, unsigned int limit);

private:
	std::vector<std::string> lines;
	unsigned int idx;
};

} // namespace newsboat

#endif /* NEWSBOAT_HISTORY_H_ */

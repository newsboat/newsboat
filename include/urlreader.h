#ifndef NEWSBOAT_URLREADER_H_
#define NEWSBOAT_URLREADER_H_

#include <map>
#include <set>
#include <string>
#include <vector>

namespace newsboat {

class UrlReader {
public:
	UrlReader() = default;
	virtual ~UrlReader() = default;
	virtual void write_config() = 0;
	virtual void reload() = 0;
	virtual std::string get_source() = 0;
	std::vector<std::string>& get_urls();
	std::vector<std::string>& get_tags(const std::string& url);
	std::vector<std::string> get_alltags();

protected:
	std::vector<std::string> urls;
	std::map<std::string, std::vector<std::string>> tags;
	std::set<std::string> alltags;
};

} // namespace newsboat

#endif /* NEWSBOAT_URLREADER_H_ */

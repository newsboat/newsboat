#ifndef NEWSBOAT_URLREADER_H_
#define NEWSBOAT_URLREADER_H_

#include <libxml/tree.h>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "configcontainer.h"

namespace newsboat {

class urlreader {
public:
	urlreader() = default;
	virtual ~urlreader() = default;
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

class opml_urlreader : public urlreader {
public:
	explicit opml_urlreader(configcontainer* c);
	void write_config() override;
	void reload() override;
	std::string get_source() override;

protected:
	virtual void handle_node(xmlNode* node, const std::string& tag);
	configcontainer* cfg;

private:
	void rec_find_rss_outlines(xmlNode* node, std::string tag);
};

} // namespace newsboat

#endif /* NEWSBOAT_URLREADER_H_ */

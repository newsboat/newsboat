#ifndef NEWSBOAT_CONFIGREADER_H_
#define NEWSBOAT_CONFIGREADER_H_

#include <libxml/tree.h>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "configcontainer.h"

namespace newsboat {

class urlreader {
public:
	urlreader();
	virtual ~urlreader();
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

class file_urlreader : public urlreader {
public:
	explicit file_urlreader(const std::string& file = "");
	~file_urlreader() override;
	void write_config() override;
	void reload() override;
	void load_config(const std::string& file);
	std::string get_source() override;

private:
	std::string filename;
};

class opml_urlreader : public urlreader {
public:
	explicit opml_urlreader(configcontainer* c);
	~opml_urlreader() override;
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

#endif /* NEWSBOAT_CONFIGREADER_H_ */

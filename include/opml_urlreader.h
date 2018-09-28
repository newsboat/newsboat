#ifndef NEWSBOAT_OPML_URLREADER_H_
#define NEWSBOAT_OPML_URLREADER_H_

#include <libxml/tree.h>
#include <string>

#include "configcontainer.h"
#include "urlreader.h"

namespace newsboat {

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

}

#endif /* NEWSBOAT_OPML_URLREADER_H_ */

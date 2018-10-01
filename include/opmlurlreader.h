#ifndef NEWSBOAT_OPMLURLREADER_H_
#define NEWSBOAT_OPMLURLREADER_H_

#include <libxml/tree.h>
#include <string>

#include "configcontainer.h"
#include "urlreader.h"

namespace newsboat {

class OpmlUrlReader : public UrlReader {
public:
	explicit OpmlUrlReader(ConfigContainer* c);
	void write_config() override;
	void reload() override;
	std::string get_source() override;

protected:
	virtual void handle_node(xmlNode* node, const std::string& tag);
	ConfigContainer* cfg;

private:
	void rec_find_rss_outlines(xmlNode* node, std::string tag);
};

}

#endif /* NEWSBOAT_OPMLURLREADER_H_ */

#ifndef NEWSBOAT_OPMLURLREADER_H_
#define NEWSBOAT_OPMLURLREADER_H_

#include <libxml/tree.h>
#include <string>

#include "configcontainer.h"
#include "urlreader.h"

namespace Newsboat {

class OpmlUrlReader : public UrlReader {
public:
	explicit OpmlUrlReader(ConfigContainer& c, const std::string& url_file);
	std::optional<utils::ReadTextFileError> reload() override;
	std::string get_source() const override;

protected:
	virtual void handle_node(xmlNode* node, const std::string& tag);
	ConfigContainer& cfg;

private:
	void rec_find_rss_outlines(xmlNode* node, std::string tag);
	std::string file;
};

}

#endif /* NEWSBOAT_OPMLURLREADER_H_ */

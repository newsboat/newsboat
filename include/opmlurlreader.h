#ifndef NEWSBOAT_OPMLURLREADER_H_
#define NEWSBOAT_OPMLURLREADER_H_

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xmlsave.h>
#include <libxml/xmlversion.h>
#include <string>

#include "feedcontainer.h"
#include "configcontainer.h"
#include "urlreader.h"

namespace newsboat {

class OpmlUrlReader : public UrlReader {
public:
	explicit OpmlUrlReader(ConfigContainer* c);
	void write_config() override;
	void reload() override;
	std::string get_source() override;
	void import(const std::string& filename);
	static void export_opml(FeedContainer& feedcontainer);

protected:
	void handle_node(const std::string& rssurl, const std::string& tag);
	ConfigContainer* cfg;

private:
	void rec_find_rss_outlines(xmlNode* node, std::string tag, bool permissive);
	static xmlDocPtr generate(const FeedContainer& feedcontainer);
};

}

#endif /* NEWSBOAT_OPMLURLREADER_H_ */

#ifndef NEWSBOAT_FRESHRSSURLREADER_H_
#define NEWSBOAT_FRESHRSSURLREADER_H_

#include "urlreader.h"

namespace Newsboat {

class ConfigContainer;
class RemoteApi;

class FreshRssUrlReader : public UrlReader {
public:
	FreshRssUrlReader(ConfigContainer* c,
		const std::string& url_file,
		RemoteApi* a);
	~FreshRssUrlReader() override;
	std::optional<utils::ReadTextFileError> reload() override;
	std::string get_source() const override;

private:
	ConfigContainer* cfg;
	std::string file;
	RemoteApi* api;
};

} // namespace Newsboat

#endif /* NEWSBOAT_FRESHRSSURLREADER_H_ */


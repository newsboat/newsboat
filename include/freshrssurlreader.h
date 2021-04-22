#ifndef NEWSBOAT_FRESHRSSURLREADER_H_
#define NEWSBOAT_FRESHRSSURLREADER_H_

#include "urlreader.h"
#include "utf8string.h"

namespace newsboat {

class ConfigContainer;
class RemoteApi;

class FreshRssUrlReader : public UrlReader {
public:
	FreshRssUrlReader(ConfigContainer* c,
		const std::string& url_file,
		RemoteApi* a);
	~FreshRssUrlReader() override;
	nonstd::optional<std::string> reload() override;
	std::string get_source() override;

private:
	ConfigContainer* cfg;
	Utf8String file;
	RemoteApi* api;
};

} // namespace newsboat

#endif /* NEWSBOAT_FRESHRSSURLREADER_H_ */


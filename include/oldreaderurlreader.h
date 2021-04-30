#ifndef NEWSBOAT_OLDREADERURLREADER_H_
#define NEWSBOAT_OLDREADERURLREADER_H_

#include "urlreader.h"
#include "utf8string.h"

namespace newsboat {

class ConfigContainer;
class RemoteApi;

class OldReaderUrlReader : public UrlReader {
public:
	OldReaderUrlReader(ConfigContainer* c,
		const std::string& url_file,
		RemoteApi* a);
	~OldReaderUrlReader() override;
	nonstd::optional<std::string> reload() override;
	std::string get_source() override;

private:
	ConfigContainer* cfg;
	Utf8String file;
	RemoteApi* api;
};

} // namespace newsboat

#endif /* NEWSBOAT_OLDREADERURLREADER_H_ */


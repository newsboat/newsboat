#ifndef NEWSBOAT_OLDREADERURLREADER_H_
#define NEWSBOAT_OLDREADERURLREADER_H_

#include "urlreader.h"

namespace newsboat {

class ConfigContainer;
class RemoteApi;

class OldReaderUrlReader : public UrlReader {
public:
	OldReaderUrlReader(ConfigContainer* c,
		const std::string& url_file,
		RemoteApi* a);
	~OldReaderUrlReader() override;
	nonstd::optional<utils::ReadTextFileError> reload() override;
	std::string get_source() override;

private:
	ConfigContainer* cfg;
	std::string file;
	RemoteApi* api;
};

} // namespace newsboat

#endif /* NEWSBOAT_OLDREADERURLREADER_H_ */


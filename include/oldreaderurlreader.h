#ifndef NEWSBOAT_OLDREADERURLREADER_H_
#define NEWSBOAT_OLDREADERURLREADER_H_

#include "urlreader.h"

namespace Newsboat {

class ConfigContainer;
class RemoteApi;

class OldReaderUrlReader : public UrlReader {
public:
	OldReaderUrlReader(ConfigContainer* c,
		const std::string& url_file,
		RemoteApi* a);
	~OldReaderUrlReader() override;
	std::optional<utils::ReadTextFileError> reload() override;
	std::string get_source() const override;

private:
	ConfigContainer* cfg;
	std::string file;
	RemoteApi* api;
};

} // namespace Newsboat

#endif /* NEWSBOAT_OLDREADERURLREADER_H_ */


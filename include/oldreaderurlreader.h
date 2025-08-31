#ifndef NEWSBOAT_OLDREADERURLREADER_H_
#define NEWSBOAT_OLDREADERURLREADER_H_

#include "urlreader.h"

namespace newsboat {

class ConfigContainer;
class RemoteApi;

class OldReaderUrlReader : public UrlReader {
public:
	OldReaderUrlReader(ConfigContainer* c,
		const Filepath& url_file,
		RemoteApi* a);
	~OldReaderUrlReader() override;
	std::optional<utils::ReadTextFileError> reload() override;
	std::string get_source() const override;

private:
	ConfigContainer* cfg;
	Filepath file;
	RemoteApi* api;
};

} // namespace newsboat

#endif /* NEWSBOAT_OLDREADERURLREADER_H_ */


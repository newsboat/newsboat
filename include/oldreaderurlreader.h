#ifndef NEWSBOAT_OLDREADERURLREADER_H_
#define NEWSBOAT_OLDREADERURLREADER_H_

#include "remoteapiurlreader.h"

namespace newsboat {

class ConfigContainer;
class RemoteApi;

class OldReaderUrlReader : public RemoteApiUrlReader {
public:
	OldReaderUrlReader(ConfigContainer& c,
		const Filepath& url_file,
		RemoteApi& a);
	~OldReaderUrlReader() override = default;
	void add_special_feeds() override;

private:
	ConfigContainer& cfg;
};

} // namespace newsboat

#endif /* NEWSBOAT_OLDREADERURLREADER_H_ */


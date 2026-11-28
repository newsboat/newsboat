#ifndef NEWSBOAT_INOREADERURLREADER_H_
#define NEWSBOAT_INOREADERURLREADER_H_

#include "remoteapiurlreader.h"

namespace newsboat {

class ConfigContainer;
class RemoteApi;

class InoreaderUrlReader : public RemoteApiUrlReader {
public:
	InoreaderUrlReader(ConfigContainer& c,
		const Filepath& url_file,
		RemoteApi& a);
	virtual ~InoreaderUrlReader() override = default;
	void add_special_feeds() override;

private:
	ConfigContainer& cfg;
};

} // namespace newsboat

#endif /* NEWSBOAT_INOREADERURLREADER_H_ */

#ifndef NEWSBOAT_FEEDHQURLREADER_H_
#define NEWSBOAT_FEEDHQURLREADER_H_

#include "remoteapiurlreader.h"

namespace newsboat {

class ConfigContainer;
class RemoteApi;

class FeedHqUrlReader : public RemoteApiUrlReader {
public:
	FeedHqUrlReader(ConfigContainer& c,
		const Filepath& url_file,
		RemoteApi& a);
	~FeedHqUrlReader() override = default;
	void add_special_feeds() override;

private:
	ConfigContainer& cfg;
};

} // namespace newsboat

#endif /* NEWSBOAT_FEEDHQURLREADER_H_ */


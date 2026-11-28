#ifndef NEWSBOAT_FRESHRSSURLREADER_H_
#define NEWSBOAT_FRESHRSSURLREADER_H_

#include "remoteapiurlreader.h"

namespace newsboat {

class ConfigContainer;
class RemoteApi;

class FreshRssUrlReader : public RemoteApiUrlReader {
public:
	FreshRssUrlReader(ConfigContainer& c,
		const Filepath& url_file,
		RemoteApi& a);
	~FreshRssUrlReader() override = default;
	void add_special_feeds() override;

private:
	ConfigContainer& cfg;
};

} // namespace newsboat

#endif /* NEWSBOAT_FRESHRSSURLREADER_H_ */


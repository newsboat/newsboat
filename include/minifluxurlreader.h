#ifndef NEWSBOAT_MINIFLUXURLREADER_H_
#define NEWSBOAT_MINIFLUXURLREADER_H_

#include "remoteapiurlreader.h"

namespace newsboat {

class RemoteApi;

class MinifluxUrlReader : public RemoteApiUrlReader {
public:
	MinifluxUrlReader(ConfigContainer& c, const Filepath& url_file, RemoteApi& a);
	~MinifluxUrlReader() override = default;
	void add_special_feeds() override;

private:
	ConfigContainer& cfg;
};

} // namespace newsboat

#endif /* NEWSBOAT_MINIFLUXURLREADER_H_ */

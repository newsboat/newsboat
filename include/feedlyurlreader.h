#ifndef NEWSBOAT_FEEDLYURLREADER_H_
#define NEWSBOAT_FEEDLYURLREADER_H_

#include "urlreader.h"

namespace newsboat {

class ConfigContainer;
class RemoteApi;

class FeedlyUrlReader : public UrlReader {
public:
	FeedlyUrlReader(ConfigContainer* c, const std::string& url_file, RemoteApi* a);
	virtual ~FeedlyUrlReader();
	virtual void reload();
	virtual std::string get_source();

private:
	ConfigContainer* cfg;
	std::string file;
	RemoteApi* api;
};

} // namespace newsboat

#endif /* NEWSBOAT_FEEDLYURLREADER_H_ */

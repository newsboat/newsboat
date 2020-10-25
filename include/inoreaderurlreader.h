#ifndef NEWSBOAT_INOREADERURLREADER_H_
#define NEWSBOAT_INOREADERURLREADER_H_

#include "urlreader.h"

namespace newsboat {

class ConfigContainer;
class RemoteApi;

class InoreaderUrlReader : public UrlReader {
public:
	InoreaderUrlReader(ConfigContainer* c,
		const std::string& url_file,
		RemoteApi* a);
	virtual ~InoreaderUrlReader();
	virtual nonstd::optional<std::string> reload();
	virtual std::string get_source();

private:
	ConfigContainer* cfg;
	std::string file;
	RemoteApi* api;
};

} // namespace newsboat

#endif /* NEWSBOAT_INOREADERURLREADER_H_ */

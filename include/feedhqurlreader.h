#ifndef NEWSBOAT_FEEDHQURLREADER_H_
#define NEWSBOAT_FEEDHQURLREADER_H_

#include "urlreader.h"

namespace Newsboat {

class ConfigContainer;
class RemoteApi;

class FeedHqUrlReader : public UrlReader {
public:
	FeedHqUrlReader(ConfigContainer* c,
		const std::string& url_file,
		RemoteApi* a);
	~FeedHqUrlReader() override;
	std::optional<utils::ReadTextFileError> reload() override;
	std::string get_source() const override;

private:
	ConfigContainer* cfg;
	std::string file;
	RemoteApi* api;
};

} // namespace Newsboat

#endif /* NEWSBOAT_FEEDHQURLREADER_H_ */


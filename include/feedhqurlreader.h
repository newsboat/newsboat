#ifndef NEWSBOAT_FEEDHQURLREADER_H_
#define NEWSBOAT_FEEDHQURLREADER_H_

#include "urlreader.h"

namespace newsboat {

class ConfigContainer;
class RemoteApi;

class FeedHqUrlReader : public UrlReader {
public:
	FeedHqUrlReader(ConfigContainer* c,
		const Filepath& url_file,
		RemoteApi* a);
	~FeedHqUrlReader() override;
	std::optional<utils::ReadTextFileError> reload() override;
	std::string get_source() const override;

private:
	ConfigContainer* cfg;
	Filepath file;
	RemoteApi* api;
};

} // namespace newsboat

#endif /* NEWSBOAT_FEEDHQURLREADER_H_ */


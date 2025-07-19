#ifndef NEWSBOAT_FRESHRSSURLREADER_H_
#define NEWSBOAT_FRESHRSSURLREADER_H_

#include "urlreader.h"

namespace newsboat {

class ConfigContainer;
class RemoteApi;

class FreshRssUrlReader : public UrlReader {
public:
	FreshRssUrlReader(ConfigContainer* c,
		const Filepath& url_file,
		RemoteApi* a);
	~FreshRssUrlReader() override;
	std::optional<utils::ReadTextFileError> reload() override;
	std::string get_source() const override;

private:
	ConfigContainer* cfg;
	Filepath file;
	RemoteApi* api;
};

} // namespace newsboat

#endif /* NEWSBOAT_FRESHRSSURLREADER_H_ */


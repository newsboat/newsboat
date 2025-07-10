#ifndef NEWSBOAT_INOREADERURLREADER_H_
#define NEWSBOAT_INOREADERURLREADER_H_

#include "urlreader.h"

namespace Newsboat {

class ConfigContainer;
class RemoteApi;

class InoreaderUrlReader : public UrlReader {
public:
	InoreaderUrlReader(ConfigContainer* c,
		const std::string& url_file,
		RemoteApi* a);
	virtual ~InoreaderUrlReader();
	virtual std::optional<utils::ReadTextFileError> reload() override;
	virtual std::string get_source() const override;

private:
	ConfigContainer* cfg;
	std::string file;
	RemoteApi* api;
};

} // namespace Newsboat

#endif /* NEWSBOAT_INOREADERURLREADER_H_ */

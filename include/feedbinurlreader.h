#ifndef NEWSBOAT_FEEDBINURLREADER_H_
#define NEWSBOAT_FEEDBINURLREADER_H_

#include "urlreader.h"

namespace newsboat {

class ConfigContainer;
class RemoteApi;

class FeedbinUrlReader : public UrlReader {
public:
	FeedbinUrlReader(ConfigContainer* c,
		const std::string& url_file,
		RemoteApi* a);
	~FeedbinUrlReader() override;
	nonstd::optional<utils::ReadTextFileError> reload() override;
	std::string get_source() override;

private:
	ConfigContainer* cfg;
	std::string file;
	RemoteApi* api;
};

} // namespace newsboat

#endif /* NEWSBOAT_FEEDBINURLREADER_H_ */


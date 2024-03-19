#ifndef NEWSBOAT_MINIFLUXURLREADER_H_
#define NEWSBOAT_MINIFLUXURLREADER_H_

#include "urlreader.h"

namespace newsboat {

class RemoteApi;

class MinifluxUrlReader : public UrlReader {
public:
	MinifluxUrlReader(ConfigContainer* c, const std::string& url_file, RemoteApi* a);
	~MinifluxUrlReader() override;
	nonstd::optional<utils::ReadTextFileError> reload() override;
	std::string get_source() override;

private:
	ConfigContainer* cfg;
	std::string file;
	RemoteApi* api;
};

} // namespace newsboat

#endif /* NEWSBOAT_MINIFLUXURLREADER_H_ */

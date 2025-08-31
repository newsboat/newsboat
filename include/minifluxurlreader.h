#ifndef NEWSBOAT_MINIFLUXURLREADER_H_
#define NEWSBOAT_MINIFLUXURLREADER_H_

#include "urlreader.h"

namespace newsboat {

class RemoteApi;

class MinifluxUrlReader : public UrlReader {
public:
	MinifluxUrlReader(ConfigContainer* c, const Filepath& url_file, RemoteApi* a);
	~MinifluxUrlReader() override;
	std::optional<utils::ReadTextFileError> reload() override;
	std::string get_source() const override;

private:
	ConfigContainer* cfg;
	Filepath file;
	RemoteApi* api;
};

} // namespace newsboat

#endif /* NEWSBOAT_MINIFLUXURLREADER_H_ */

#ifndef NEWSBOAT_MINIFLUXURLREADER_H_
#define NEWSBOAT_MINIFLUXURLREADER_H_

#include "urlreader.h"

namespace newsboat {

class RemoteApi;

class MinifluxUrlReader : public UrlReader {
public:
	MinifluxUrlReader(const std::string& url_file, RemoteApi* a);
	~MinifluxUrlReader() override;
	void reload() override;
	std::string get_source() override;

private:
	std::string file;
	RemoteApi* api;
};

} // namespace newsboat

#endif /* NEWSBOAT_MINIFLUXURLREADER_H_ */

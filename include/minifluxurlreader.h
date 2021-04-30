#ifndef NEWSBOAT_MINIFLUXURLREADER_H_
#define NEWSBOAT_MINIFLUXURLREADER_H_

#include "urlreader.h"

#include "utf8string.h"

namespace newsboat {

class RemoteApi;

class MinifluxUrlReader : public UrlReader {
public:
	MinifluxUrlReader(const std::string& url_file, RemoteApi* a);
	~MinifluxUrlReader() override;
	nonstd::optional<std::string> reload() override;
	std::string get_source() override;

private:
	Utf8String file;
	RemoteApi* api;
};

} // namespace newsboat

#endif /* NEWSBOAT_MINIFLUXURLREADER_H_ */

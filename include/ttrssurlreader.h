#ifndef NEWSBOAT_TTRSSURLREADER_H_
#define NEWSBOAT_TTRSSURLREADER_H_

#include "urlreader.h"
#include "utf8string.h"

namespace newsboat {

class RemoteApi;

class TtRssUrlReader : public UrlReader {
public:
	TtRssUrlReader(const std::string& url_file, RemoteApi* a);
	~TtRssUrlReader() override;
	nonstd::optional<std::string> reload() override;
	std::string get_source() override;

private:
	Utf8String file;
	RemoteApi* api;
};

} // namespace newsboat

#endif /* NEWSBOAT_TTRSSURLREADER_H_ */

#ifndef NEWSBOAT_NEWSBLURURLREADER_H_
#define NEWSBOAT_NEWSBLURURLREADER_H_

#include "rss/feed.h"
#include "urlreader.h"
#include "utf8string.h"

namespace newsboat {

class RemoteApi;

class NewsBlurUrlReader : public UrlReader {
public:
	NewsBlurUrlReader(const std::string& url_file, RemoteApi* a);
	~NewsBlurUrlReader() override;
	nonstd::optional<std::string> reload() override;
	std::string get_source() override;

private:
	Utf8String file;
	RemoteApi* api;
};

} // namespace newsboat

#endif /* NEWSBOAT_NEWSBLURURLREADER_H_ */


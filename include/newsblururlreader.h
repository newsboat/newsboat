#ifndef NEWSBOAT_NEWSBLURURLREADER_H_
#define NEWSBOAT_NEWSBLURURLREADER_H_

#include "rss/feed.h"
#include "urlreader.h"

#define ID_SEPARATOR "/////"

namespace newsboat {

class RemoteApi;

typedef std::map<std::string, rsspp::Feed> FeedMap;

class NewsBlurUrlReader : public UrlReader {
public:
	NewsBlurUrlReader(const Filepath& url_file, RemoteApi* a);
	~NewsBlurUrlReader() override;
	nonstd::optional<utils::ReadTextFileError> reload() override;
	std::string get_source() override;

private:
	Filepath file;
	RemoteApi* api;
};

} // namespace newsboat

#endif /* NEWSBOAT_NEWSBLURURLREADER_H_ */


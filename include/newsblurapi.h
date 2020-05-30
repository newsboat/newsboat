#ifndef NEWSBOAT_NEWSBLURAPI_H_
#define NEWSBOAT_NEWSBLURAPI_H_

#include <json-c/json.h>

#include "remoteapi.h"
#include "rss/feed.h"

#define ID_SEPARATOR "/////"

namespace newsboat {

typedef std::map<std::string, rsspp::Feed> FeedMap;

class NewsBlurApi : public RemoteApi {
public:
	explicit NewsBlurApi(ConfigContainer* c);
	~NewsBlurApi() override;
	bool authenticate() override;
	std::vector<TaggedFeedUrl> get_subscribed_urls() override;
	void add_custom_headers(curl_slist** custom_headers) override;
	bool mark_all_read(const std::string& feedurl) override;
	bool mark_article_read(const std::string& guid, bool read) override;
	bool update_article_flags(const std::string& oldflags,
		const std::string& newflags,
		const std::string& guid) override;
	rsspp::Feed fetch_feed(const std::string& id);
	// TODO
private:
	std::string retrieve_auth();
	json_object* query_api(const std::string& url,
		const std::string* postdata);
	std::map<std::string, std::vector<std::string>> mk_feeds_to_tags(
			json_object*);
	std::string api_location;
	FeedMap known_feeds;
	unsigned int min_pages;
};

} // namespace newsboat

#endif /* NEWSBOAT_NEWSBLURAPI_H_ */

#ifndef NEWSBOAT_FEEDLYAPI_H_
#define NEWSBOAT_FEEDLYAPI_H_

#include <json-c/json.h>

#include <unordered_map>
#include "remoteapi.h"
#include "rss/feed.h"

namespace newsboat {

class FeedlyApi : public RemoteApi {
public:
	explicit FeedlyApi(ConfigContainer* c);
	virtual ~FeedlyApi();
	virtual bool authenticate();
	virtual std::vector<TaggedFeedUrl> get_subscribed_urls();
	virtual void add_custom_headers(curl_slist** custom_headers);
	virtual bool mark_all_read(const std::string& feedurl);
	virtual bool mark_article_read(const std::string& guid, bool read);
	virtual bool update_article_flags(const std::string& oldflags,
		const std::string& newflags,
		const std::string& guid);
	rsspp::Feed fetch_feed(const std::string& id);

private:
	std::string star_flag;
	std::string auth_header;
	std::string request_url(const std::string& url,
		const std::string* postdata = nullptr);
	bool star_article(const std::string& guid, bool star);
	std::unordered_map<std::string, TaggedFeedUrl> get_all_feeds();
};

} // namespace newsboat

#endif /* NEWSBOAT_FEEDLYAPI_H_ */

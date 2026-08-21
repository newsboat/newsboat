#ifndef NEWSBOAT_CACHEAPI_H_
#define NEWSBOAT_CACHEAPI_H_

#include "remoteapi.h"
#include "cache.h"

namespace newsboat {
class CacheApi : public RemoteApi {
public:
	explicit CacheApi(ConfigContainer& cfg, Cache* cache);
	~CacheApi() override = default;
	bool authenticate() override;
	std::vector<TaggedFeedUrl> get_subscribed_urls() override;
	bool mark_all_read(const std::string& feedurl) override;
	bool mark_article_read(const std::string& guid, bool read) override;
	bool update_article_flags(const std::string& oldflags,
		const std::string& newflags,
		const std::string& guid) override;
	void add_custom_headers(curl_slist**) override;

private:
	Cache* cache;
};
}

#endif /* NEWSBOAT_CACHEAPI_H_ */

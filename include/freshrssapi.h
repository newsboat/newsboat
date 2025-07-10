#ifndef NEWSBOAT_FRESHRSSAPI_H_
#define NEWSBOAT_FRESHRSSAPI_H_

#include <libxml/tree.h>

#include "remoteapi.h"
#include "rss/feed.h"
#include "utils.h"

class CurlHandle;

using HTTPMethod = Newsboat::utils::HTTPMethod;

namespace Newsboat {

class FreshRssApi : public RemoteApi {
public:
	explicit FreshRssApi(ConfigContainer& c);
	~FreshRssApi() override = default;
	bool authenticate() override;
	std::vector<TaggedFeedUrl> get_subscribed_urls() override;
	void add_custom_headers(curl_slist** custom_headers) override;
	bool mark_all_read(const std::string& feedurl) override;
	bool mark_article_read(const std::string& guid, bool read) override;
	bool update_article_flags(const std::string& oldflags,
		const std::string& newflags,
		const std::string& guid) override;
	rsspp::Feed fetch_feed(const std::string& id, CurlHandle& cached_handle);

private:
	std::vector<std::string> get_tags(xmlNode* node);
	std::string get_new_token();
	bool refresh_token();
	std::string retrieve_auth();
	std::string post_content(const std::string& url,
		const std::string& postdata);
	bool star_article(const std::string& guid, bool star);
	bool share_article(const std::string& guid, bool share);
	bool mark_article_read_with_token(const std::string& guid,
		bool read,
		const std::string& token);
	std::string auth;
	std::string auth_header;
	bool token_expired;
	std::string token;
};

} // namespace Newsboat

#endif /* NEWSBOAT_FRESHRSSAPI_H_ */

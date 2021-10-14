#ifndef NEWSBOAT_MINIFLUXAPI_H_
#define NEWSBOAT_MINIFLUXAPI_H_

#include "3rd-party/json.hpp"
#include "curlhandle.h"
#include "remoteapi.h"
#include "rss/feed.h"
#include "utils.h"

using HTTPMethod = newsboat::utils::HTTPMethod;

namespace newsboat {

class MinifluxApi : public RemoteApi {
public:
	explicit MinifluxApi(ConfigContainer* cfg);
	~MinifluxApi() override;
	bool authenticate() override;
	std::vector<TaggedFeedUrl> get_subscribed_urls() override;
	bool mark_all_read(const std::string& feedurl) override;
	bool mark_article_read(const std::string& guid, bool read) override;
	bool update_article_flags(const std::string& oldflags,
		const std::string& newflags,
		const std::string& guid) override;
	void add_custom_headers(curl_slist**) override;
	rsspp::Feed fetch_feed(const std::string& id, CurlHandle* cached_handle);

private:
	virtual nlohmann::json run_op(const std::string& path,
		const nlohmann::json& req_data,
		const HTTPMethod method = HTTPMethod::GET,
		CurlHandle* cached_handle = nullptr);
	TaggedFeedUrl feed_from_json(const nlohmann::json& jfeed,
		const std::vector<std::string>& tags);
	bool flag_changed(const std::string& oldflags,
		const std::string& newflags,
		const std::string& flagstr);
	bool update_articles(const std::vector<std::string> guids,
		nlohmann::json& args);
	bool update_article(const std::string& guid, nlohmann::json& args);
	std::string auth_info;
	std::string server;
};

} // namespace newsboat

#endif /* NEWSBOAT_MINIFLUXAPI_H_ */

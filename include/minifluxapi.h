#ifndef NEWSBOAT_MINIFLUXAPI_H_
#define NEWSBOAT_MINIFLUXAPI_H_

#include "3rd-party/json.hpp"
#include "remoteapi.h"
#include "rss/feed.h"
#include "utils.h"

using HTTPMethod = newsboat::utils::HTTPMethod;

namespace newsboat {

class CurlHandle;

class MinifluxApi : public RemoteApi {
public:
	explicit MinifluxApi(ConfigContainer& cfg);
	~MinifluxApi() override = default;
	bool authenticate() override;
	std::vector<TaggedFeedUrl> get_subscribed_urls() override;
	bool mark_all_read(const std::string& feedurl) override;
	bool mark_article_read(const std::string& guid, bool read) override;
	bool update_article_flags(const std::string& oldflags,
		const std::string& newflags,
		const std::string& guid) override;
	void add_custom_headers(curl_slist**) override;
	rsspp::Feed fetch_feed(const std::string& id, CurlHandle& easyhandle);

private:
	virtual nlohmann::json run_op(const std::string& path,
		const nlohmann::json& req_data,
		const HTTPMethod method = HTTPMethod::GET);
	virtual nlohmann::json run_op(const std::string& path,
		const nlohmann::json& req_data,
		CurlHandle& cached_handle,
		const HTTPMethod method = HTTPMethod::GET);
	TaggedFeedUrl feed_from_json(const nlohmann::json& jfeed,
		const std::vector<std::string>& tags);
	bool flag_changed(const std::string& oldflags,
		const std::string& newflags,
		const std::string& flagstr);
	bool update_articles(const std::vector<std::string> guids,
		nlohmann::json& args);
	bool update_article(const std::string& guid, nlohmann::json& args);
	bool star_article(const std::string& guid, bool star);
	bool save_article(const std::string& guid, bool save);
	std::string auth_info;
	std::string auth_token;
	std::string server;
};

} // namespace newsboat

#endif /* NEWSBOAT_MINIFLUXAPI_H_ */

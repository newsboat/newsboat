#ifndef NEWSBOAT_FEEDBINAPI_H_
#define NEWSBOAT_FEEDBINAPI_H_

#include "remoteapi.h"
#include "rss/feed.h"
#include "3rd-party/json.hpp"
#include "utils.h"

class CurlHandle;

using HTTPMethod = Newsboat::utils::HTTPMethod;

namespace Newsboat {

class FeedbinApi : public RemoteApi {
public:
	explicit FeedbinApi(ConfigContainer& c);
	~FeedbinApi() override = default;
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
	virtual nlohmann::json run_op(const std::string& path,
		const nlohmann::json& req_data,
		const HTTPMethod method = HTTPMethod::GET);
	virtual nlohmann::json run_op(const std::string& path,
		const nlohmann::json& req_data,
		CurlHandle& cached_handle,
		const HTTPMethod method = HTTPMethod::GET);
	bool star_article(const std::string& guid, bool star);
	bool mark_entries_read(const std::vector<std::string>& ids, bool read);
	TaggedFeedUrl feed_from_json(const nlohmann::json& jfeed,
		const std::vector<std::string>& tags);
	std::string auth_info;
};

} // namespace Newsboat

#endif /* NEWSBOAT_FEEDBINAPI_H_ */

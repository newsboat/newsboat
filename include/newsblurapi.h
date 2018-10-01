#ifndef NEWSBOAT_NEWSBLURAPI_H_
#define NEWSBOAT_NEWSBLURAPI_H_

#include <json.h>

#include "remoteapi.h"
#include "rsspp.h"
#include "urlreader.h"

#define ID_SEPARATOR "/////"

namespace newsboat {

typedef std::map<std::string, rsspp::feed> feedmap;

class NewsBlurApi : public RemoteApi {
public:
	explicit NewsBlurApi(ConfigContainer* c);
	~NewsBlurApi() override;
	bool authenticate() override;
	std::vector<tagged_feedurl> get_subscribed_urls() override;
	void add_custom_headers(curl_slist** custom_headers) override;
	bool mark_all_read(const std::string& feedurl) override;
	bool mark_article_read(const std::string& guid, bool read) override;
	bool update_article_flags(const std::string& oldflags,
		const std::string& newflags,
		const std::string& guid) override;
	rsspp::feed fetch_feed(const std::string& id);
	// TODO
private:
	std::string retrieve_auth();
	json_object* query_api(const std::string& url,
		const std::string* postdata);
	std::map<std::string, std::vector<std::string>> mk_feeds_to_tags(
		json_object*);
	std::string api_location;
	feedmap known_feeds;
	unsigned int min_pages;
};

class NewsBlurUrlReader : public UrlReader {
public:
	NewsBlurUrlReader(const std::string& url_file, RemoteApi* a);
	~NewsBlurUrlReader() override;
	void write_config() override;
	void reload() override;
	std::string get_source() override;

private:
	std::string file;
	RemoteApi* api;
};

} // namespace newsboat

#endif /* NEWSBOAT_NEWSBLURAPI_H_ */

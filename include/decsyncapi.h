#ifndef NEWSBOAT_DECSYNCAPI_H_
#define NEWSBOAT_DECSYNCAPI_H_

#include <libdecsync.h>

#include "remoteapi.h"
#include "fileurlreader.h"
#include "rssitem.h"

namespace newsboat {

class DecsyncApi : public RemoteApi {
public:
	explicit DecsyncApi(ConfigContainer* c);
	~DecsyncApi() override;
	bool authenticate() override;
	std::vector<TaggedFeedUrl> get_subscribed_urls() override;
	void add_custom_headers(curl_slist** custom_headers) override;
	bool mark_all_read(const std::string& feedurl) override;
	bool mark_article_read(const std::string& guid, bool read) override;
	bool update_article_flags(const std::string& oldflags,
		const std::string& newflags,
		const std::string& guid) override;

private:
	void add_read_entry(std::vector<DecsyncEntryWithPath>& entries, const RssItem& item, bool read);
	void subscribe(std::string feedUrl, bool subscribed);

	std::vector<TaggedFeedUrl> subscribed_urls;
	Decsync decsync;
};

class DecsyncUrlReader : public UrlReader {
public:
	DecsyncUrlReader(const std::string& url_file, RemoteApi* a);
	void write_config() override;
	void reload() override;
	std::string get_source() override;

private:
	std::string file;
	RemoteApi* api;
};

} // namespace newsboat

#endif /* NEWSBOAT_DECSYNCAPI_H_ */

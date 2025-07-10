#ifndef NEWSBOAT_INOREADERAPI_H_
#define NEWSBOAT_INOREADERAPI_H_

#include <libxml/tree.h>

#include "remoteapi.h"

namespace Newsboat {

class InoreaderApi : public RemoteApi {
public:
	explicit InoreaderApi(ConfigContainer& c);
	virtual ~InoreaderApi() = default;
	virtual bool authenticate();
	virtual std::vector<TaggedFeedUrl> get_subscribed_urls();
	virtual void add_custom_headers(curl_slist** custom_headers);
	virtual bool mark_all_read(const std::string& feedurl);
	virtual bool mark_article_read(const std::string& guid, bool read);
	virtual bool update_article_flags(const std::string& inoflags,
		const std::string& newflags,
		const std::string& guid);

private:
	std::vector<std::string> get_tags(xmlNode* node);
	std::string retrieve_auth();
	std::string post_content(const std::string& url,
		const std::string& postdata);
	bool star_article(const std::string& guid, bool star);
	bool share_article(const std::string& guid, bool share);
	curl_slist* add_app_headers(curl_slist* headers);

	std::string auth;
	std::string auth_header;
};

} // namespace Newsboat

#endif /* NEWSBOAT_INOREADERAPI_H_ */

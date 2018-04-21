#ifndef NEWSBOAT_INOREADER_API_H_
#define NEWSBOAT_INOREADER_API_H_

#include "cache.h"
#include "remote_api.h"
#include "urlreader.h"

namespace newsboat {

class inoreader_api : public remote_api {
public:
	explicit inoreader_api(configcontainer* c);
	virtual ~inoreader_api();
	virtual bool authenticate();
	virtual std::vector<tagged_feedurl> get_subscribed_urls();
	virtual void add_custom_headers(curl_slist** custom_headers);
	virtual bool mark_all_read(const std::string& feedurl);
	virtual bool mark_article_read(const std::string& guid, bool read);
	virtual bool update_article_flags(
		const std::string& inoflags,
		const std::string& newflags,
		const std::string& guid);

private:
	std::vector<std::string> get_tags(xmlNode* node);
	std::string get_new_token();
	std::string retrieve_auth();
	std::string
	post_content(const std::string& url, const std::string& postdata);
	bool star_article(const std::string& guid, bool star);
	bool share_article(const std::string& guid, bool share);
	bool mark_article_read_with_token(
		const std::string& guid,
		bool read,
		const std::string& token);
	std::string auth;
	std::string auth_header;
};

class inoreader_urlreader : public urlreader {
public:
	inoreader_urlreader(
		configcontainer* c,
		const std::string& url_file,
		remote_api* a);
	virtual ~inoreader_urlreader();
	virtual void write_config();
	virtual void reload();
	virtual std::string get_source();

private:
	configcontainer* cfg;
	std::string file;
	remote_api* api;
};

} // namespace newsboat

#endif /* NEWSBOAT_INOREADER_API_H_ */

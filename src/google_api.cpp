#include <vector>
#include <cstring>

#include <google_api.h>
#include <config.h>
#include <utils.h>

#include <curl/curl.h>

#define GREADER_LOGIN "https://www.google.com/accounts/ClientLogin"
#define GREADER_SUBSCRIPTION_LIST "http://www.google.com/reader/api/0/subscription/list"
#define GREADER_FEED_PREFIX "http://www.google.com/reader/atom/"

// for reference, see http://code.google.com/p/pyrfeed/wiki/GoogleReaderAPI

namespace newsbeuter {

googlereader_api::googlereader_api(configcontainer * c) : remote_api(c) {
	// TODO
}

googlereader_api::~googlereader_api() {
	// TODO
}

bool googlereader_api::authenticate() {
	sid = retrieve_sid();
	LOG(LOG_DEBUG, "googlereader_api::authenticate: SID = %s", sid.c_str());
	return sid != "";
}

static size_t my_write_data(void *buffer, size_t size, size_t nmemb, void *userp) {
	std::string * pbuf = static_cast<std::string *>(userp);
	pbuf->append(static_cast<const char *>(buffer), size * nmemb);
	return size * nmemb;
}

std::string googlereader_api::retrieve_sid() {
	CURL * handle = curl_easy_init();
	std::string post_content = utils::strprintf("service=reader&Email=%s&Passwd=%s&source=%s/%s&continue=http://www.google.com/", 
		cfg->get_configvalue("googlereader-login").c_str(), cfg->get_configvalue("googlereader-password").c_str(), PROGRAM_NAME, PROGRAM_VERSION);
	std::string result;
	
	utils::set_common_curl_options(handle, cfg);
	curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, my_write_data);
	curl_easy_setopt(handle, CURLOPT_WRITEDATA, &result);
	curl_easy_setopt(handle, CURLOPT_POSTFIELDS, post_content.c_str());
	curl_easy_setopt(handle, CURLOPT_URL, GREADER_LOGIN);
	curl_easy_perform(handle);
	curl_easy_cleanup(handle);

	std::vector<std::string> lines = utils::tokenize(result);
	for (std::vector<std::string>::iterator it=lines.begin();it!=lines.end();it++) {
		LOG(LOG_DEBUG, "googlereader_api::retrieve_sid: line = %s", it->c_str());
		if (it->substr(0,4)=="SID=") {
			std::string sid = it->substr(4, it->length()-4);
			return sid;
		}
	}

	return "";
}

std::vector<tagged_feedurl> googlereader_api::get_subscribed_urls() {
	std::vector<tagged_feedurl> urls;

	CURL * handle = curl_easy_init();
	std::string result;
	std::string cookie = utils::strprintf("SID=%s;", sid.c_str());
	
	utils::set_common_curl_options(handle, cfg);
	curl_easy_setopt(handle, CURLOPT_COOKIE, cookie.c_str());
	curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, my_write_data);
	curl_easy_setopt(handle, CURLOPT_WRITEDATA, &result);
	curl_easy_setopt(handle, CURLOPT_URL, GREADER_SUBSCRIPTION_LIST);
	curl_easy_perform(handle);
	curl_easy_cleanup(handle);

	LOG(LOG_DEBUG, "googlereader_api::get_subscribed_urls: document = %s", result.c_str());

	xmlDoc * doc = xmlParseMemory(result.c_str(), result.length());

	if (!doc) {
		LOG(LOG_ERROR, "googlreader_api::get_subscribed_urls: failed to parse subscription list");
		return urls;
	}

	xmlNode * root = xmlDocGetRootElement(doc);

	LOG(LOG_DEBUG, "googlereader_api::get_subscribed_urls: root = %p", root);

	if (root) {
		for (xmlNode * node = root->children; node != NULL; node = node->next) {
			if (strcmp((const char *)node->name, "list")==0 && utils::get_prop(node, "name") == "subscriptions") {
				LOG(LOG_DEBUG, "found subscriptions list");
				for (xmlNode * objectnode = node->children; objectnode != NULL; objectnode = objectnode->next) {
					if (strcmp((const char *)objectnode->name, "object")==0) {
						LOG(LOG_DEBUG, "found object");
						std::string id;
						std::vector<std::string> tags;
						for (xmlNode * elem = objectnode->children; elem != NULL; elem = elem->next) {
							if (strcmp((const char *)elem->name, "string")==0 && utils::get_prop(elem,"name")=="id") {
								LOG(LOG_DEBUG, "found id");
								id = utils::get_content(elem);
							} else if (strcmp((const char *)elem->name, "list")==0 && utils::get_prop(elem,"name")=="categories") {
								LOG(LOG_DEBUG, "found tag");
								tags = get_tags(elem);
							}
						}
						if (id != "") {
							urls.push_back(tagged_feedurl(utils::strprintf("%s%s", GREADER_FEED_PREFIX, utils::replace_all(id, "?", "%3F").c_str()), tags));
						}
					}
				}
			}
		}
	}

	xmlFreeDoc(doc);

	return urls;
}

std::vector<std::string> googlereader_api::get_tags(xmlNode * node) {
	std::vector<std::string> tags;

	if (node && strcmp((const char *)node->name, "list")==0 && utils::get_prop(node, "name")=="categories") {
		LOG(LOG_DEBUG, "found tag list");
		for (xmlNode * objnode = node->children; objnode != NULL; objnode = objnode->next) {
			if (strcmp((const char *)objnode->name, "object")==0) {
				LOG(LOG_DEBUG, "found tag list object");
				for (xmlNode * str = objnode->children; str != NULL; str = str->next) {
					if (strcmp((const char *)str->name, "string")==0 && utils::get_prop(str, "name")=="label") {
						LOG(LOG_DEBUG, "found tag list label");
						tags.push_back(utils::get_content(str));
					}
				}
			}
		}
	}
	return tags;
}

void googlereader_api::configure_handle(CURL * handle) {
	std::string cookie = utils::strprintf("SID=%s;", sid.c_str());
	curl_easy_setopt(handle, CURLOPT_COOKIE, cookie.c_str());
}

}

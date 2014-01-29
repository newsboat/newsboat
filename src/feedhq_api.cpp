#include <vector>
#include <cstring>
#include <iostream>
#include <wordexp.h>

#include <feedhq_api.h>
#include <config.h>
#include <utils.h>

#include <unistd.h>

#include <curl/curl.h>
#include <json.h>

#define FEEDHQ_LOGIN					"https://feedhq.org/accounts/ClientLogin"
#define FEEDHQ_API_PREFIX				"https://feedhq.org/reader/api/0/"
#define FEEDHQ_FEED_PREFIX				"https://feedhq.org/reader/atom/"

#define FEEDHQ_OUTPUT_SUFFIX "?output=json"

#define FEEDHQ_SUBSCRIPTION_LIST		FEEDHQ_API_PREFIX "subscription/list" FEEDHQ_OUTPUT_SUFFIX
#define FEEDHQ_API_MARK_ALL_READ_URL	FEEDHQ_API_PREFIX "mark-all-as-read"
#define FEEDHQ_API_EDIT_TAG_URL		FEEDHQ_API_PREFIX "edit-tag"
#define FEEDHQ_API_TOKEN_URL			FEEDHQ_API_PREFIX "token"

namespace newsbeuter {

feedhq_api::feedhq_api(configcontainer * c) : remote_api(c) {
	// TODO
}

feedhq_api::~feedhq_api() {
	// TODO
}

bool feedhq_api::authenticate() {
	auth = retrieve_auth();
	LOG(LOG_DEBUG, "feedhq_api::authenticate: Auth = %s", auth.c_str());
	return auth != "";
}

static size_t my_write_data(void *buffer, size_t size, size_t nmemb, void *userp) {
	std::string * pbuf = static_cast<std::string *>(userp);
	pbuf->append(static_cast<const char *>(buffer), size * nmemb);
	return size * nmemb;
}

std::string feedhq_api::retrieve_auth() {
	CURL * handle = curl_easy_init();
        std::string user = cfg->get_configvalue("feedhq-login");
        bool flushed = false;

        if (user == "") {
                std::cout << std::endl;
                std::cout.flush();
                flushed = true;
                std::cout << "Username for FeedHQ: ";
                std::cin >> user;
                if (user == "") {
                    return "";
                }
        }

	std::string pass = cfg->get_configvalue("feedhq-password");
	if( pass == "" ) {
		wordexp_t exp;
		std::ifstream ifs;
		wordexp(cfg->get_configvalue("feedhq-passwordfile").c_str(),&exp,0);
		ifs.open(exp.we_wordv[0]); 
		wordfree(&exp);
		if (!ifs) {
                        if(!flushed) {
                            std::cout << std::endl;
                            std::cout.flush();
                        }
			// Find a way to do this in C++ by removing cin echoing.
			pass = std::string( getpass("Password for FeedHQ: ") );
		} else {
				ifs >> pass;
				if(pass == "") {
						return "";
				}
		}
	}
	char * username = curl_easy_escape(handle, user.c_str(), 0);
	char * password = curl_easy_escape(handle, pass.c_str(), 0);

	std::string postcontent = utils::strprintf("service=reader&Email=%s&Passwd=%s&source=%s%2F%s&accountType=HOSTED_OR_GOOGLE&continue=http://www.google.com/",
		username, password, PROGRAM_NAME, PROGRAM_VERSION);

	curl_free(username);
	curl_free(password);

	std::string result;

	utils::set_common_curl_options(handle, cfg);
	curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, my_write_data);
	curl_easy_setopt(handle, CURLOPT_WRITEDATA, &result);
	curl_easy_setopt(handle, CURLOPT_POSTFIELDS, postcontent.c_str());
	curl_easy_setopt(handle, CURLOPT_URL, FEEDHQ_LOGIN);
	curl_easy_perform(handle);
	curl_easy_cleanup(handle);

	std::vector<std::string> lines = utils::tokenize(result);
	for (std::vector<std::string>::iterator it=lines.begin();it!=lines.end();++it) {
		LOG(LOG_DEBUG, "feedhq_api::retrieve_auth: line = %s", it->c_str());
		if (it->substr(0,5)=="Auth=") {
			std::string auth = it->substr(5, it->length()-5);
			return auth;
		}
	}

	return "";
}

std::vector<tagged_feedurl> feedhq_api::get_subscribed_urls() {
	std::vector<tagged_feedurl> urls;

	CURL * handle = curl_easy_init();
	std::string result;
	configure_handle(handle);

	utils::set_common_curl_options(handle, cfg);
	curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, my_write_data);
	curl_easy_setopt(handle, CURLOPT_WRITEDATA, &result);
	curl_easy_setopt(handle, CURLOPT_URL, FEEDHQ_SUBSCRIPTION_LIST);
	curl_easy_perform(handle);
	curl_easy_cleanup(handle);

	LOG(LOG_DEBUG, "feedhq_api::get_subscribed_urls: document = %s", result.c_str());

	// TODO: parse result
	
	struct json_object * reply = json_tokener_parse(result.c_str());
	if (is_error(reply)) {
		LOG(LOG_ERROR, "feedhq_api::get_subscribed_urls: failed to parse response as JSON.");
		return urls;
	}


	struct json_object * subscription_obj = json_object_object_get(reply, "subscriptions");
	struct array_list * subscriptions = json_object_get_array(subscription_obj);

	int len = array_list_length(subscriptions);

	for (int i=0;i<len;i++) {
		std::vector<std::string> tags;
		struct json_object * sub = json_object_array_get_idx(subscription_obj, i);

		const char * id = json_object_get_string(json_object_object_get(sub, "id"));
		const char * title = json_object_get_string(json_object_object_get(sub, "title"));
		tags.push_back(std::string("~") + title);

		char * escaped_id = curl_easy_escape(handle, id, 0);

		urls.push_back(tagged_feedurl(utils::strprintf("%s%s?n=%u", FEEDHQ_FEED_PREFIX, escaped_id, cfg->get_configvalue_as_int("feedhq-min-items")), tags));

		curl_free(escaped_id);
	}

	json_object_put(reply);

	return urls;
}

void feedhq_api::configure_handle(CURL * handle) {
	struct curl_slist *chunk = NULL;
	std::string header = utils::strprintf("Authorization: GoogleLogin auth=%s", auth.c_str());
	LOG(LOG_DEBUG, "feedhq_api::configure_handle header = %s", header.c_str());
	chunk = curl_slist_append(chunk, header.c_str());
	curl_easy_setopt(handle, CURLOPT_HTTPHEADER, chunk);
}

bool feedhq_api::mark_all_read(const std::string& feedurl) {
	std::string real_feedurl = feedurl.substr(strlen(FEEDHQ_FEED_PREFIX), feedurl.length() - strlen(FEEDHQ_FEED_PREFIX));
	std::vector<std::string> elems = utils::tokenize(real_feedurl, "?");
	real_feedurl = utils::unescape_url(elems[0]);
	std::string token = get_new_token();

	std::string postcontent = utils::strprintf("s=%s&T=%s", real_feedurl.c_str(), token.c_str());
	
	std::string result = post_content(FEEDHQ_API_MARK_ALL_READ_URL, postcontent);

	return result == "OK";
}

std::vector<std::string> feedhq_api::bulk_mark_articles_read(const std::vector<google_replay_pair>& actions) {
	std::vector<std::string> successful_tokens;
	std::string token = get_new_token();
	for (std::vector<google_replay_pair>::const_iterator it=actions.begin();it!=actions.end();++it) {
		bool read;
		if (it->second == GOOGLE_MARK_READ) {
			read = true;
		} else if (it->second == GOOGLE_MARK_UNREAD) {
			read = false;
		} else {
			continue;
		}
		if (mark_article_read_with_token(it->first, read, token)) {
			successful_tokens.push_back(it->first);
		}
	}
	return successful_tokens;
}

bool feedhq_api::mark_article_read(const std::string& guid, bool read) {
	std::string token = get_new_token();
	return mark_article_read_with_token(guid, read, token);
}

bool feedhq_api::mark_article_read_with_token(const std::string& guid, bool read, const std::string& token) {
	std::string postcontent;

	if (read) {
		postcontent = utils::strprintf("i=%s&a=user/-/state/com.google/read&r=user/-/state/com.google/kept-unread&ac=edit&T=%s", guid.c_str(), token.c_str());
	} else {
		postcontent = utils::strprintf("i=%s&r=user/-/state/com.google/read&a=user/-/state/com.google/kept-unread&a=user/-/state/com.google/tracking-kept-unread&ac=edit&T=%s", guid.c_str(), token.c_str());
	}

	std::string result = post_content(FEEDHQ_API_EDIT_TAG_URL, postcontent);

	LOG(LOG_DEBUG, "feedhq_api::mark_article_read_with_token: postcontent = %s result = %s", postcontent.c_str(), result.c_str());

	return result == "OK";
}

std::string feedhq_api::get_new_token() {
	CURL * handle = curl_easy_init();
	std::string result;
	
	utils::set_common_curl_options(handle, cfg);
	configure_handle(handle);
	curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, my_write_data);
	curl_easy_setopt(handle, CURLOPT_WRITEDATA, &result);
	curl_easy_setopt(handle, CURLOPT_URL, FEEDHQ_API_TOKEN_URL);
	curl_easy_perform(handle);
	curl_easy_cleanup(handle);

	LOG(LOG_DEBUG, "feedhq_api::get_new_token: token = %s", result.c_str());
	
	return result;
}

bool feedhq_api::update_article_flags(const std::string& oldflags, const std::string& newflags, const std::string& guid) {
	std::string star_flag = cfg->get_configvalue("feedhq-flag-star");
	std::string share_flag = cfg->get_configvalue("feedhq-flag-share");
	bool success = true;

	if (star_flag.length() > 0) {
		if (strchr(oldflags.c_str(), star_flag[0])==NULL && strchr(newflags.c_str(), star_flag[0])!=NULL) {
			success = star_article(guid, true);
		} else if (strchr(oldflags.c_str(), star_flag[0])!=NULL && strchr(newflags.c_str(), star_flag[0])==NULL) {
			success = star_article(guid, false);
		}
	}

	if (share_flag.length() > 0) {
		if (strchr(oldflags.c_str(), share_flag[0])==NULL && strchr(newflags.c_str(), share_flag[0])!=NULL) {
			success = share_article(guid, true);
		} else if (strchr(oldflags.c_str(), share_flag[0])!=NULL && strchr(newflags.c_str(), share_flag[0])==NULL) {
			success = share_article(guid, false);
		}
	}

	return success;
}

bool feedhq_api::star_article(const std::string& guid, bool star) {
	std::string token = get_new_token();
	std::string postcontent;

	if (star) {
		postcontent = utils::strprintf("i=%s&a=user/-/state/com.google/starred&ac=edit&T=%s", guid.c_str(), token.c_str());
	} else {
		postcontent = utils::strprintf("i=%s&r=user/-/state/com.google/starred&ac=edit&T=%s", guid.c_str(), token.c_str());
	}

	std::string result = post_content(FEEDHQ_API_EDIT_TAG_URL, postcontent);

	return result == "OK";
}

bool feedhq_api::share_article(const std::string& guid, bool share) {
	std::string token = get_new_token();
	std::string postcontent;

	if (share) {
		postcontent = utils::strprintf("i=%s&a=user/-/state/com.google/broadcast&ac=edit&T=%s", guid.c_str(), token.c_str());
	} else {
		postcontent = utils::strprintf("i=%s&r=user/-/state/com.google/broadcast&ac=edit&T=%s", guid.c_str(), token.c_str());
	}

	std::string result = post_content(FEEDHQ_API_EDIT_TAG_URL, postcontent);

	return result == "OK";
}

std::string feedhq_api::post_content(const std::string& url, const std::string& postdata) {
	std::string result;

	CURL * handle = curl_easy_init();
	utils::set_common_curl_options(handle, cfg);
	configure_handle(handle);
	curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, my_write_data);
	curl_easy_setopt(handle, CURLOPT_WRITEDATA, &result);
	curl_easy_setopt(handle, CURLOPT_POSTFIELDS, postdata.c_str());
	curl_easy_setopt(handle, CURLOPT_URL, url.c_str());
	curl_easy_perform(handle);
	curl_easy_cleanup(handle);

	LOG(LOG_DEBUG, "feedhq_api::post_content: url = %s postdata = %s result = %s", url.c_str(), postdata.c_str(), result.c_str());

	return result;
}

}

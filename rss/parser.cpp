/* rsspp - Copyright (C) 2008-2012 Andreas Krennmair <ak@newsbeuter.org>
 * Licensed under the MIT/X Consortium License. See file LICENSE
 * for more information.
 */

#include <config.h>
#include <rsspp.h>
#include <rsspp_internal.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <curl/curl.h>
#include <logger.h>
#include <utils.h>
#include <cstring>
#include <utils.h>
#include <remote_api.h>

using namespace newsbeuter;

static size_t my_write_data(void *buffer, size_t size, size_t nmemb, void *userp) {
	std::string * pbuf = static_cast<std::string *>(userp);
	pbuf->append(static_cast<const char *>(buffer), size * nmemb);
	return size * nmemb;
}

namespace rsspp {

parser::parser(unsigned int timeout, const char * user_agent, const char * proxy, const char * proxy_auth, curl_proxytype proxy_type) 
	: to(timeout), ua(user_agent), prx(proxy), prxauth(proxy_auth), prxtype(proxy_type), doc(0), lm(0) {
}

parser::~parser() {
	if (doc)
		xmlFreeDoc(doc);
}

struct header_values {
	time_t lastmodified;
	std::string etag;

	header_values()
		: lastmodified(0)
	{
	}
};

static size_t handle_headers(void * ptr, size_t size, size_t nmemb, void * data) {
	char * header = new char[size*nmemb + 1];
	header_values * values = (header_values *)data;

	memcpy(header, ptr, size*nmemb);
	header[size*nmemb] = '\0';

	if (!strncasecmp("Last-Modified:", header, 14)) {
		time_t r = curl_getdate(header+14, NULL);
		if (r == -1) {
			LOG(LOG_DEBUG, "handle_headers: last-modified %s (curl_getdate FAILED)", header+14);
		} else {
			values->lastmodified = curl_getdate(header+14, NULL);
			LOG(LOG_DEBUG, "handle_headers: got last-modified %s (%d)", header+14, values->lastmodified);
		}
	} else if (!strncasecmp("ETag:",header, 5)) {
		values->etag = std::string(header+5);
		utils::trim(values->etag);
		LOG(LOG_DEBUG, "handle_headers: got etag %s", values->etag.c_str());
	}

	delete[] header;

	return size * nmemb;
}

feed parser::parse_url(const std::string& url, time_t lastmodified, const std::string& etag, newsbeuter::remote_api * api, const std::string& cookie_cache, CURL *ehandle) {
	std::string buf;
	CURLcode ret;

	CURL * easyhandle = ehandle;
	if (!easyhandle) {
		easyhandle = curl_easy_init();
		if (!easyhandle) {
			throw exception(_("couldn't initialize libcurl"));
		}
	}

	if (ua) {
		curl_easy_setopt(easyhandle, CURLOPT_USERAGENT, ua);
	}
	if (api) {
		api->configure_handle(easyhandle);
	}
	curl_easy_setopt(easyhandle, CURLOPT_URL, url.c_str());
	curl_easy_setopt(easyhandle, CURLOPT_SSL_VERIFYPEER, 0);
	curl_easy_setopt(easyhandle, CURLOPT_WRITEFUNCTION, my_write_data);
	curl_easy_setopt(easyhandle, CURLOPT_WRITEDATA, &buf);
	curl_easy_setopt(easyhandle, CURLOPT_NOSIGNAL, 1);
	curl_easy_setopt(easyhandle, CURLOPT_FOLLOWLOCATION, 1);
	curl_easy_setopt(easyhandle, CURLOPT_MAXREDIRS, 10);
	curl_easy_setopt(easyhandle, CURLOPT_FAILONERROR, 1);
	curl_easy_setopt(easyhandle, CURLOPT_ENCODING, "gzip, deflate");
	if (cookie_cache != "") {
		curl_easy_setopt(easyhandle, CURLOPT_COOKIEFILE, cookie_cache.c_str());
		curl_easy_setopt(easyhandle, CURLOPT_COOKIEJAR, cookie_cache.c_str());
	}
	if (to != 0)
		curl_easy_setopt(easyhandle, CURLOPT_TIMEOUT, to);

	if (prx)
		curl_easy_setopt(easyhandle, CURLOPT_PROXY, prx);

	if (prxauth) {
		curl_easy_setopt(easyhandle, CURLOPT_PROXYAUTH, CURLAUTH_ANY);
		curl_easy_setopt(easyhandle, CURLOPT_PROXYUSERPWD, prxauth);
	}

	curl_easy_setopt(easyhandle, CURLOPT_PROXYTYPE, prxtype);

	header_values hdrs;
	curl_easy_setopt(easyhandle, CURLOPT_HEADERDATA, &hdrs);
	curl_easy_setopt(easyhandle, CURLOPT_HEADERFUNCTION, handle_headers);

	curl_easy_setopt(easyhandle, CURLOPT_TIMECONDITION, CURL_TIMECOND_IFMODSINCE);
	if (lastmodified != 0)
		curl_easy_setopt(easyhandle, CURLOPT_TIMEVALUE, lastmodified);
	else
		curl_easy_setopt(easyhandle, CURLOPT_TIMEVALUE, 0);

	curl_slist * custom_headers = NULL;
	if (etag.length() > 0) {
		custom_headers = curl_slist_append(custom_headers, utils::strprintf("If-None-Match: %s", etag.c_str()).c_str());
		curl_easy_setopt(easyhandle, CURLOPT_HTTPHEADER, custom_headers);
	}

	ret = curl_easy_perform(easyhandle);

	lm = hdrs.lastmodified;
	et = hdrs.etag;

	if (custom_headers) {
		curl_easy_setopt(easyhandle, CURLOPT_HTTPHEADER, 0);
		curl_slist_free_all(custom_headers);
	}

	LOG(LOG_DEBUG, "rsspp::parser::parse_url: ret = %d", ret);

	long status;
	curl_easy_getinfo(easyhandle, CURLINFO_HTTP_CONNECTCODE, &status);

	if (status >= 400) {
		LOG(LOG_USERERROR, _("Error: trying to download feed `%s' returned HTTP status code %ld."), url.c_str(), status);
	}

	if (!ehandle)
		curl_easy_cleanup(easyhandle);

	if (ret != 0) {
		LOG(LOG_ERROR, "rsspp::parser::parse_url: curl_easy_perform returned err %d: %s", ret, curl_easy_strerror(ret));
		throw exception(curl_easy_strerror(ret));
	}

	LOG(LOG_INFO, "parser::parse_url: retrieved data for %s: %s", url.c_str(), buf.c_str());

	if (buf.length() > 0) {
		LOG(LOG_DEBUG, "parser::parse_url: handing over data to parse_buffer()");
		return parse_buffer(buf.c_str(), buf.length(), url.c_str());
	}

	return feed();
}

feed parser::parse_buffer(const char * buffer, size_t size, const char * url) {
	doc = xmlReadMemory(buffer, size, url, NULL, XML_PARSE_RECOVER | XML_PARSE_NOERROR | XML_PARSE_NOWARNING);
	if (doc == NULL) {
		throw exception(_("could not parse buffer"));
	}

	xmlNode* root_element = xmlDocGetRootElement(doc);

	feed f = parse_xmlnode(root_element);

	if (doc->encoding) {
		f.encoding = (const char *)doc->encoding;
	}

	LOG(LOG_INFO, "parser::parse_buffer: encoding = %s", f.encoding.c_str());

	return f;
}

feed parser::parse_file(const std::string& filename) {
	doc = xmlReadFile(filename.c_str(), NULL, XML_PARSE_RECOVER | XML_PARSE_NOERROR | XML_PARSE_NOWARNING);
	if (doc == NULL) {
		throw exception(_("could not parse file"));
	}

	xmlNode* root_element = xmlDocGetRootElement(doc);

	feed f = parse_xmlnode(root_element);

	if (doc->encoding) {
		f.encoding = (const char *)doc->encoding;
	}

	LOG(LOG_INFO, "parser::parse_file: encoding = %s", f.encoding.c_str());

	return f;
}

feed parser::parse_xmlnode(xmlNode* node) {
	feed f;

	if (node) {
		if (node->name && node->type == XML_ELEMENT_NODE) {
			if (strcmp((const char *)node->name, "rss")==0) {
				const char * version = (const char *)xmlGetProp(node, (const xmlChar *)"version");
				if (!version) {
					xmlFree((void *)version);
					throw exception(_("no RSS version"));
				}
				if (strcmp(version, "0.91")==0)
					f.rss_version = RSS_0_91;
				else if (strcmp(version, "0.92")==0)
					f.rss_version = RSS_0_92;
				else if (strcmp(version, "0.94")==0)
					f.rss_version = RSS_0_94;
				else if (strcmp(version, "2.0")==0 || strcmp(version, "2")==0)
					f.rss_version = RSS_2_0;
				else {
					xmlFree((void *)version);
					throw exception(_("invalid RSS version"));
				}
				xmlFree((void *)version);
			} else if (strcmp((const char *)node->name, "RDF")==0) {
				f.rss_version = RSS_1_0;
			} else if (strcmp((const char *)node->name, "feed")==0) {
				if (node->ns && node->ns->href) {
					if (strcmp((const char *)node->ns->href, ATOM_0_3_URI)==0) {
						f.rss_version = ATOM_0_3;
					} else if (strcmp((const char *)node->ns->href, ATOM_1_0_URI)==0) {
						f.rss_version = ATOM_1_0;
					} else {
						const char * version = (const char *)xmlGetProp(node, (const xmlChar *)"version");
						if (!version) {
							xmlFree((void *)version);
							throw exception(_("invalid Atom version"));
						}
						if (strcmp(version, "0.3")==0) {
							xmlFree((void *)version);
							f.rss_version = ATOM_0_3_NONS;
						} else {
							xmlFree((void *)version);
							throw exception(_("invalid Atom version"));
						}
					}
				} else {
					throw exception(_("no Atom version"));
				}
			}

			std::tr1::shared_ptr<rss_parser> parser = rss_parser_factory::get_object(f, doc);

			try {
				parser->parse_feed(f, node);
			} catch (exception& e) {
				throw;
			}
		}
	} else {
		throw exception(_("XML root node is NULL"));
	}

	return f;
}

void parser::global_init() {
	LIBXML_TEST_VERSION
	curl_global_init(CURL_GLOBAL_ALL);
}

void parser::global_cleanup() {
	xmlCleanupParser();
	curl_global_cleanup();
}


}

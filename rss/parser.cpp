/* rsspp - Copyright (C) 2008 Andreas Krennmair <ak@newsbeuter.org>
 * Licensed under the MIT/X Consortium License. See file LICENSE
 * for more information.
 */

#include <rsspp.h>
#include <rsspp_internal.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <curl/curl.h>
#include <logger.h>
#include <cstring>

using namespace newsbeuter;

static size_t my_write_data(void *buffer, size_t size, size_t nmemb, void *userp) {
	std::string * pbuf = static_cast<std::string *>(userp);
	pbuf->append(static_cast<const char *>(buffer), size * nmemb);
	return size * nmemb;
}

namespace rsspp {

parser::parser(unsigned int timeout, const char * user_agent, const char * proxy, const char * proxy_auth) 
	: to(timeout), ua(user_agent), prx(proxy), prxauth(proxy_auth), doc(0) {
}

parser::~parser() {
	if (doc)
		xmlFreeDoc(doc);
	xmlCleanupParser();
}

feed parser::parse_url(const std::string& url) {
	std::string buf;
	CURLcode ret;

	CURL * easyhandle = curl_easy_init();
	if (!easyhandle) {
		throw exception(0, "curl_easy_init() failed");
	}

	if (ua) {
		curl_easy_setopt(easyhandle, CURLOPT_USERAGENT, ua);
	}
	curl_easy_setopt(easyhandle, CURLOPT_URL, url.c_str());
	curl_easy_setopt(easyhandle, CURLOPT_WRITEFUNCTION, my_write_data);
	curl_easy_setopt(easyhandle, CURLOPT_WRITEDATA, &buf);
	curl_easy_setopt(easyhandle, CURLOPT_NOSIGNAL, 1);
	curl_easy_setopt(easyhandle, CURLOPT_FOLLOWLOCATION, 1);
	curl_easy_setopt(easyhandle, CURLOPT_FAILONERROR, 1);
	curl_easy_setopt(easyhandle, CURLOPT_ENCODING, "gzip, deflate");
	if (to != 0)
		curl_easy_setopt(easyhandle, CURLOPT_TIMEOUT, to);

	if (prx)
		curl_easy_setopt(easyhandle, CURLOPT_PROXY, prx);

	if (prxauth)
		curl_easy_setopt(easyhandle, CURLOPT_PROXYUSERPWD, prxauth);

	ret = curl_easy_perform(easyhandle);
	curl_easy_cleanup(easyhandle);

	GetLogger().log(LOG_DEBUG, "rsspp::parser::parse_url: ret = %d", ret);

	if (ret != 0) {
		throw exception(0, "curl_easy_perform error");
	}

	GetLogger().log(LOG_INFO, "parser::parse_url: retrieved data for %s: %s", url.c_str(), buf.c_str());

	if (buf.length() > 0)
		return parse_buffer(buf.c_str(), buf.length());

	return feed();
}

feed parser::parse_buffer(const char * buffer, size_t size, const char * url) {
	doc = xmlReadMemory(buffer, size, url, NULL, XML_PARSE_RECOVER | XML_PARSE_NOERROR | XML_PARSE_NOWARNING);
	if (doc == NULL) {
		throw exception(0, "unable to parse buffer");
	}

	xmlNode* root_element = xmlDocGetRootElement(doc);

	feed f = parse_xmlnode(root_element);

	if (doc->encoding) {
		f.encoding = (const char *)doc->encoding;
	}

	GetLogger().log(LOG_INFO, "parser::parse_buffer: encoding = %s", f.encoding.c_str());

	return f;
}

feed parser::parse_file(const std::string& filename) {
	doc = xmlReadFile(filename.c_str(), NULL, 0);
	if (doc == NULL) {
		throw exception(0, "unable to parse file");
	}

	xmlNode* root_element = xmlDocGetRootElement(doc);

	feed f = parse_xmlnode(root_element);

	if (doc->encoding) {
		f.encoding = (const char *)doc->encoding;
	}

	GetLogger().log(LOG_INFO, "parser::parse_file: encoding = %s", f.encoding.c_str());

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
					throw exception(0, "no RSS version");
				}
				if (strcmp(version, "0.91")==0)
					f.rss_version = RSS_0_91;
				else if (strcmp(version, "0.92")==0)
					f.rss_version = RSS_0_92;
				else if (strcmp(version, "2.0")==0)
					f.rss_version = RSS_2_0;
				else {
					xmlFree((void *)version);
					throw exception(0, "invalid RSS version");
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
						throw exception(0, "invalid Atom version");
					}
				} else {
					throw exception(0, "no Atom version");
				}
			}

			std::tr1::shared_ptr<rss_parser> parser = rss_parser_factory::get_object(f);

			try {
				parser->parse_feed(f, node);
			} catch (exception& e) {
				throw e;
			}
		}
	} else {
		// TODO: throw exception
	}

	return f;
}

void parser::global_init() {
	LIBXML_TEST_VERSION
}

void parser::global_cleanup() {
	xmlCleanupParser();
}


}

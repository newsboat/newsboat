#include "rsspp.h"

#include <cstring>
#include <curl/curl.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

#include "config.h"
#include "logger.h"
#include "remote_api.h"
#include "rsspp_internal.h"
#include "strprintf.h"
#include "utils.h"

using namespace newsboat;

static size_t
my_write_data(void* buffer, size_t size, size_t nmemb, void* userp)
{
	std::string* pbuf = static_cast<std::string*>(userp);
	pbuf->append(static_cast<const char*>(buffer), size * nmemb);
	return size * nmemb;
}

namespace rsspp {

parser::parser(
	unsigned int timeout,
	const std::string& user_agent,
	const std::string& proxy,
	const std::string& proxy_auth,
	curl_proxytype proxy_type,
	const bool ssl_verify)
	: to(timeout)
	, ua(user_agent)
	, prx(proxy)
	, prxauth(proxy_auth)
	, prxtype(proxy_type)
	, verify_ssl(ssl_verify)
	, doc(0)
	, lm(0)
{
}

parser::~parser()
{
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

static size_t handle_headers(void* ptr, size_t size, size_t nmemb, void* data)
{
	char* header = new char[size * nmemb + 1];
	header_values* values = static_cast<header_values*>(data);

	memcpy(header, ptr, size * nmemb);
	header[size * nmemb] = '\0';

	if (!strncasecmp("Last-Modified:", header, 14)) {
		time_t r = curl_getdate(header + 14, nullptr);
		if (r == -1) {
			LOG(level::DEBUG,
			    "handle_headers: last-modified %s (curl_getdate "
			    "FAILED)",
			    header + 14);
		} else {
			values->lastmodified =
				curl_getdate(header + 14, nullptr);
			LOG(level::DEBUG,
			    "handle_headers: got last-modified %s (%d)",
			    header + 14,
			    values->lastmodified);
		}
	} else if (!strncasecmp("ETag:", header, 5)) {
		values->etag = std::string(header + 5);
		utils::trim(values->etag);
		LOG(level::DEBUG, "handle_headers: got etag %s", values->etag);
	}

	delete[] header;

	return size * nmemb;
}

feed parser::parse_url(
	const std::string& url,
	time_t lastmodified,
	const std::string& etag,
	newsboat::remote_api* api,
	const std::string& cookie_cache,
	CURL* ehandle)
{
	std::string buf;
	CURLcode ret;
	curl_slist* custom_headers{};

	CURL* easyhandle = ehandle;
	if (!easyhandle) {
		easyhandle = curl_easy_init();
		if (!easyhandle) {
			throw exception(_("couldn't initialize libcurl"));
		}
	}

	if (!ua.empty()) {
		curl_easy_setopt(easyhandle, CURLOPT_USERAGENT, ua.c_str());
	}

	if (api) {
		api->add_custom_headers(&custom_headers);
	}
	curl_easy_setopt(easyhandle, CURLOPT_URL, url.c_str());
	curl_easy_setopt(easyhandle, CURLOPT_SSL_VERIFYPEER, verify_ssl);
	curl_easy_setopt(easyhandle, CURLOPT_WRITEFUNCTION, my_write_data);
	curl_easy_setopt(easyhandle, CURLOPT_WRITEDATA, &buf);
	curl_easy_setopt(easyhandle, CURLOPT_NOSIGNAL, 1);
	curl_easy_setopt(easyhandle, CURLOPT_FOLLOWLOCATION, 1);
	curl_easy_setopt(easyhandle, CURLOPT_MAXREDIRS, 10);
	curl_easy_setopt(easyhandle, CURLOPT_FAILONERROR, 1);
	curl_easy_setopt(easyhandle, CURLOPT_ACCEPT_ENCODING, "gzip, deflate");
	if (cookie_cache != "") {
		curl_easy_setopt(
			easyhandle, CURLOPT_COOKIEFILE, cookie_cache.c_str());
		curl_easy_setopt(
			easyhandle, CURLOPT_COOKIEJAR, cookie_cache.c_str());
	}
	if (to != 0)
		curl_easy_setopt(easyhandle, CURLOPT_TIMEOUT, to);

	if (!prx.empty())
		curl_easy_setopt(easyhandle, CURLOPT_PROXY, prx.c_str());

	if (!prxauth.empty()) {
		curl_easy_setopt(easyhandle, CURLOPT_PROXYAUTH, CURLAUTH_ANY);
		curl_easy_setopt(
			easyhandle, CURLOPT_PROXYUSERPWD, prxauth.c_str());
	}

	curl_easy_setopt(easyhandle, CURLOPT_PROXYTYPE, prxtype);

	const char* curl_ca_bundle = ::getenv("CURL_CA_BUNDLE");
	if (curl_ca_bundle != nullptr) {
		curl_easy_setopt(easyhandle, CURLOPT_CAINFO, curl_ca_bundle);
	}

	header_values hdrs;
	curl_easy_setopt(easyhandle, CURLOPT_HEADERDATA, &hdrs);
	curl_easy_setopt(easyhandle, CURLOPT_HEADERFUNCTION, handle_headers);

	if (lastmodified != 0) {
		curl_easy_setopt(
			easyhandle,
			CURLOPT_TIMECONDITION,
			CURL_TIMECOND_IFMODSINCE);
		curl_easy_setopt(easyhandle, CURLOPT_TIMEVALUE, lastmodified);
	}

	if (etag.length() > 0) {
		auto header = strprintf::fmt("If-None-Match: %s", etag);
		custom_headers =
			curl_slist_append(custom_headers, header.c_str());
	}

	if (lastmodified != 0 || etag.length() > 0) {
		custom_headers =
			curl_slist_append(custom_headers, "A-IM: feed");
	}

	if (custom_headers) {
		curl_easy_setopt(
			easyhandle, CURLOPT_HTTPHEADER, custom_headers);
	}

	ret = curl_easy_perform(easyhandle);

	lm = hdrs.lastmodified;
	et = hdrs.etag;

	if (custom_headers) {
		curl_easy_setopt(easyhandle, CURLOPT_HTTPHEADER, 0);
		curl_slist_free_all(custom_headers);
	}

	LOG(level::DEBUG,
	    "rsspp::parser::parse_url: ret = %d (%s)",
	    ret,
	    curl_easy_strerror(ret));

	long status;
	CURLcode infoOk =
		curl_easy_getinfo(easyhandle, CURLINFO_RESPONSE_CODE, &status);

	curl_easy_reset(easyhandle);
	if (cookie_cache != "") {
		curl_easy_setopt(
			easyhandle, CURLOPT_COOKIEJAR, cookie_cache.c_str());
	}

	if (!ehandle)
		curl_easy_cleanup(easyhandle);

	if (ret != 0) {
		LOG(level::ERROR,
		    "rsspp::parser::parse_url: curl_easy_perform returned err "
		    "%d: %s",
		    ret,
		    curl_easy_strerror(ret));
		std::string msg;
		if (ret == CURLE_HTTP_RETURNED_ERROR && infoOk == CURLE_OK) {
			msg = strprintf::fmt(
				"%s %li", curl_easy_strerror(ret), status);
		} else {
			msg = curl_easy_strerror(ret);
		}
		throw exception(msg);
	}

	LOG(level::INFO,
	    "parser::parse_url: retrieved data for %s: %s",
	    url,
	    buf);

	if (buf.length() > 0) {
		LOG(level::DEBUG,
		    "parser::parse_url: handing over data to parse_buffer()");
		return parse_buffer(buf, url);
	}

	return feed();
}

feed parser::parse_buffer(const std::string& buffer, const std::string& url)
{
	doc = xmlReadMemory(
		buffer.c_str(),
		buffer.length(),
		url.c_str(),
		nullptr,
		XML_PARSE_RECOVER | XML_PARSE_NOERROR | XML_PARSE_NOWARNING);
	if (doc == nullptr) {
		throw exception(_("could not parse buffer"));
	}

	xmlNode* root_element = xmlDocGetRootElement(doc);

	feed f = parse_xmlnode(root_element);

	if (doc->encoding) {
		f.encoding = (const char*)doc->encoding;
	}

	LOG(level::INFO, "parser::parse_buffer: encoding = %s", f.encoding);

	return f;
}

feed parser::parse_file(const std::string& filename)
{
	doc = xmlReadFile(
		filename.c_str(),
		nullptr,
		XML_PARSE_RECOVER | XML_PARSE_NOERROR | XML_PARSE_NOWARNING);
	xmlNode* root_element = xmlDocGetRootElement(doc);

	if (root_element == nullptr) {
		throw exception(_("could not parse file"));
	}

	feed f = parse_xmlnode(root_element);

	if (doc->encoding) {
		f.encoding = (const char*)doc->encoding;
	}

	LOG(level::INFO, "parser::parse_file: encoding = %s", f.encoding);

	return f;
}

feed parser::parse_xmlnode(xmlNode* node)
{
	feed f;

	if (node) {
		if (node->name && node->type == XML_ELEMENT_NODE) {
			if (strcmp((const char*)node->name, "rss") == 0) {
				const char* version = (const char*)xmlGetProp(
					node, (const xmlChar*)"version");
				if (!version) {
					throw exception(_("no RSS version"));
				}
				if (strcmp(version, "0.91") == 0)
					f.rss_version = RSS_0_91;
				else if (strcmp(version, "0.92") == 0)
					f.rss_version = RSS_0_92;
				else if (strcmp(version, "0.94") == 0)
					f.rss_version = RSS_0_94;
				else if (
					strcmp(version, "2.0") == 0
					|| strcmp(version, "2") == 0)
					f.rss_version = RSS_2_0;
				else if (strcmp(version, "1.0") == 0)
					f.rss_version = RSS_0_91;
				else {
					xmlFree((void*)version);
					throw exception(
						_("invalid RSS version"));
				}
				xmlFree((void*)version);
			} else if (
				strcmp((const char*)node->name, "RDF") == 0) {
				f.rss_version = RSS_1_0;
			} else if (
				strcmp((const char*)node->name, "feed") == 0) {
				if (node->ns && node->ns->href) {
					if (strcmp((const char*)node->ns->href,
						   ATOM_0_3_URI)
					    == 0) {
						f.rss_version = ATOM_0_3;
					} else if (
						strcmp((const char*)
							       node->ns->href,
						       ATOM_1_0_URI)
						== 0) {
						f.rss_version = ATOM_1_0;
					} else {
						const char * version = (const char *)xmlGetProp(node, (const xmlChar *)"version");
						if (!version) {
							throw exception(_(
								"invalid Atom "
								"version"));
						}
						if (strcmp(version, "0.3")
						    == 0) {
							xmlFree((void*)version);
							f.rss_version =
								ATOM_0_3_NONS;
						} else {
							xmlFree((void*)version);
							throw exception(_(
								"invalid Atom "
								"version"));
						}
					}
				} else {
					throw exception(_("no Atom version"));
				}
			}

			std::shared_ptr<rss_parser> parser =
				rss_parser_factory::get_object(f, doc);

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

void parser::global_init()
{
	LIBXML_TEST_VERSION
	curl_global_init(CURL_GLOBAL_ALL);
}

void parser::global_cleanup()
{
	xmlCleanupParser();
	curl_global_cleanup();
}

} // namespace rsspp

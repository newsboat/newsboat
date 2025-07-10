#include "feedretriever.h"

#include <cinttypes>

#include "cache.h"
#include "config.h"
#include "configcontainer.h"
#include "curlhandle.h"
#include "feedbinapi.h"
#include "freshrssapi.h"
#include "logger.h"
#include "minifluxapi.h"
#include "newsblurapi.h"
#include "ocnewsapi.h"
#include "remoteapi.h"
#include "rss/parser.h"
#include "rssignores.h"
#include "strprintf.h"
#include "ttrssapi.h"
#include "utils.h"

namespace Newsboat {

FeedRetriever::FeedRetriever(ConfigContainer& cfg, Cache& ch, CurlHandle&
	easyhandle, RssIgnores* ign, RemoteApi* api)
	: cfg(cfg)
	, ch(ch)
	, ign(ign)
	, api(api)
	, easyhandle(easyhandle)
{
}

rsspp::Feed FeedRetriever::retrieve(const std::string& uri)
{
	/*
	 *	- http:// and https:// URLs are downloaded and parsed regularly
	 *	- exec: URLs are executed and their output is parsed
	 *	- filter: URLs are downloaded, executed, and their output is
	 *parsed
	 *	- query: URLs are ignored
	 */
	const std::string urls_source = cfg.get_configvalue("urls-source");
	if (urls_source == "ttrss") {
		const std::string::size_type pound = uri.find_first_of('#');
		if (pound != std::string::npos) {
			return fetch_ttrss(uri.substr(pound + 1));
		} else {
			return {};
		}
	} else if (urls_source == "newsblur") {
		return fetch_newsblur(uri);
	} else if (urls_source == "ocnews") {
		return fetch_ocnews(uri);
	} else if (urls_source == "miniflux") {
		return fetch_miniflux(uri);
	} else if (urls_source == "feedbin") {
		const std::string::size_type pound = uri.find_first_of('#');
		if (pound != std::string::npos) {
			const std::string feed_id = uri.substr(pound + 1);
			return fetch_feedbin(feed_id);
		} else {
			return {};
		}
	} else if (urls_source == "freshrss") {
		return fetch_freshrss(uri);
	} else if (utils::is_http_url(uri)) {
		return download_http(uri);
	} else if (utils::is_exec_url(uri)) {
		return get_execplugin(uri.substr(5, uri.length() - 5));
	} else if (utils::is_filter_url(uri)) {
		const auto parts = utils::extract_filter(uri);
		return download_filterplugin(std::string(parts.script_name), std::string(parts.url));
	} else if (utils::is_query_url(uri)) {
		return {};
	} else if (uri.substr(0, 7) == "file://") {
		return parse_file(uri.substr(7, uri.length() - 7));
	} else {
		throw strprintf::fmt(_("Error: unsupported URL: %s"), uri);
	}
}

rsspp::Feed FeedRetriever::fetch_ttrss(const std::string& feed_id)
{
	rsspp::Feed f;
	TtRssApi* tapi = dynamic_cast<TtRssApi*>(api);
	if (tapi) {
		f = tapi->fetch_feed(feed_id, easyhandle);
	}
	LOG(Level::DEBUG,
		"FeedRetriever::fetch_ttrss: f.items.size = %" PRIu64,
		static_cast<uint64_t>(f.items.size()));

	return f;
}

rsspp::Feed FeedRetriever::fetch_newsblur(const std::string& feed_id)
{
	rsspp::Feed f;
	NewsBlurApi* napi = dynamic_cast<NewsBlurApi*>(api);
	if (napi) {
		f = napi->fetch_feed(feed_id);
	}
	LOG(Level::INFO,
		"FeedRetriever::fetch_newsblur: f.items.size = %" PRIu64,
		static_cast<uint64_t>(f.items.size()));

	return f;
}

rsspp::Feed FeedRetriever::fetch_ocnews(const std::string& feed_id)
{
	rsspp::Feed f;
	OcNewsApi* napi = dynamic_cast<OcNewsApi*>(api);
	if (napi) {
		f = napi->fetch_feed(feed_id);
	}
	LOG(Level::INFO,
		"FeedRetriever::fetch_ocnews: f.items.size = %" PRIu64,
		static_cast<uint64_t>(f.items.size()));

	return f;
}

rsspp::Feed FeedRetriever::fetch_miniflux(const std::string& feed_id)
{
	rsspp::Feed f;
	MinifluxApi* mapi = dynamic_cast<MinifluxApi*>(api);
	if (mapi) {
		f = mapi->fetch_feed(feed_id, easyhandle);
	}
	LOG(Level::INFO,
		"FeedRetriever::fetch_miniflux: f.items.size = %" PRIu64,
		static_cast<uint64_t>(f.items.size()));

	return f;
}

rsspp::Feed FeedRetriever::fetch_feedbin(const std::string& feed_id)
{
	rsspp::Feed f;
	FeedbinApi* fapi = dynamic_cast<FeedbinApi*>(api);
	if (fapi) {
		f = fapi->fetch_feed(feed_id, easyhandle);
	}
	LOG(Level::INFO,
		"FeedRetriever::fetch_feedbin: f.items.size = %" PRIu64,
		static_cast<uint64_t>(f.items.size()));

	return f;
}

rsspp::Feed FeedRetriever::fetch_freshrss(const std::string& feed_id)
{
	rsspp::Feed f;
	FreshRssApi* fapi = dynamic_cast<FreshRssApi*>(api);
	if (fapi) {
		f = fapi->fetch_feed(feed_id, easyhandle);
	}
	LOG(Level::INFO,
		"FeedRetriever::fetch_freshrss: f.items.size = %" PRIu64,
		static_cast<uint64_t>(f.items.size()));

	return f;
}

rsspp::Feed FeedRetriever::download_http(const std::string& uri)
{
	rsspp::Feed f;
	const unsigned int retrycount = cfg.get_configvalue_as_int("download-retries");
	std::string proxy;
	std::string proxy_auth;
	std::string proxy_type;

	if (cfg.get_configvalue_as_bool("use-proxy") == true) {
		proxy = cfg.get_configvalue("proxy");
		proxy_auth = cfg.get_configvalue("proxy-auth");
		proxy_type = cfg.get_configvalue("proxy-type");
	}

	for (unsigned int i = 0; i < retrycount
		&& f.rss_version == rsspp::Feed::Version::UNKNOWN; i++) {
		std::string useragent = utils::get_useragent(cfg);
		LOG(Level::DEBUG,
			"FeedRetriever::download_http: user-agent = %s",
			useragent);
		rsspp::Parser p(cfg.get_configvalue_as_int(
				"download-timeout"),
			useragent,
			proxy,
			proxy_auth,
			utils::get_proxy_type(proxy_type),
			cfg.get_configvalue_as_bool(
				"ssl-verifypeer"));
		time_t lm = 0;
		std::string etag;
		if (!ign || !ign->matches_lastmodified(uri)) {
			ch.fetch_lastmodified(uri, lm, etag);
		}
		f = p.parse_url(uri,
				easyhandle,
				lm,
				etag,
				api,
				cfg.get_configvalue("cookie-cache"));
		LOG(Level::DEBUG,
			"FeedRetriever::download_http: lm = %" PRId64 " etag = %s",
			// On GCC, `time_t` is `long int`, which is at least 32 bits
			// long according to the spec. On x86_64, it's actually 64
			// bits. Thus, casting to int64_t is either a no-op, or an
			// up-cast which are always safe.
			static_cast<int64_t>(p.get_last_modified()),
			p.get_etag());
		if (p.get_last_modified() != 0 ||
			p.get_etag().length() > 0) {
			LOG(Level::DEBUG,
				"FeedRetriever::download_http: "
				"lastmodified "
				"old: %" PRId64 " new: %" PRId64,
				// On GCC, `time_t` is `long int`, which is at least 32
				// bits long according to the spec. On x86_64, it's
				// actually 64 bits. Thus, casting to int64_t is either
				// a no-op, or an up-cast which are always safe.
				static_cast<int64_t>(lm),
				static_cast<int64_t>(p.get_last_modified()));
			LOG(Level::DEBUG,
				"FeedRetriever::download_http: etag old: "
				"%s "
				"new %s",
				etag,
				p.get_etag());
			ch.update_lastmodified(uri,
				(p.get_last_modified() != lm)
				? p.get_last_modified()
				: 0,
				(etag != p.get_etag()) ? p.get_etag()
				: "");
		}
	}
	LOG(Level::DEBUG,
		"FeedRetriever::download_http: http URL %s, valid: %s",
		uri,
		(f.rss_version != rsspp::Feed::Version::UNKNOWN) ? "true" : "false");

	return f;
}

rsspp::Feed FeedRetriever::get_execplugin(const std::string& plugin)
{
	std::string buf = utils::get_command_output(plugin);
	rsspp::Parser p;
	const rsspp::Feed f = p.parse_buffer(buf);
	LOG(Level::DEBUG,
		"FeedRetriever::get_execplugin: execplugin %s, valid = %s",
		plugin,
		(f.rss_version != rsspp::Feed::Version::UNKNOWN) ? "true" : "false");

	return f;
}

rsspp::Feed FeedRetriever::download_filterplugin(const std::string& filter,
	const std::string& uri)
{
	std::string buf = utils::retrieve_url(uri, cfg);

	const char* argv[4] = {"/bin/sh",
			"-c",
			filter.c_str(),
			nullptr
		};
	const std::string result = utils::run_program(argv, buf);
	LOG(Level::DEBUG,
		"FeedRetriever::download_filterplugin: output of `%s' is: %s",
		filter,
		result);
	rsspp::Parser p;
	const rsspp::Feed f = p.parse_buffer(result);
	LOG(Level::DEBUG,
		"FeedRetriever::download_filterplugin: filterplugin %s, valid = %s",
		filter,
		(f.rss_version != rsspp::Feed::Version::UNKNOWN) ? "true" : "false");

	return f;
}

rsspp::Feed FeedRetriever::parse_file(const std::string& file)
{
	rsspp::Parser p;
	const rsspp::Feed f = p.parse_file(file);
	LOG(Level::DEBUG,
		"FeedRetriever::parse: parsed file %s, valid = %s",
		file,
		(f.rss_version != rsspp::Feed::Version::UNKNOWN) ? "true" : "false");

	return f;
}


} // namespace Newsboat

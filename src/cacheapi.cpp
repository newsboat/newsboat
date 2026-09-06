#include "../include/cacheapi.h"
#include "../include/feedorigin.h"
#include "cacheapi.h"

namespace newsboat {

CacheApi::CacheApi(ConfigContainer& cfg, Cache* cache) : RemoteApi(cfg), cache(cache)
{

}

bool CacheApi::authenticate()
{
	return false;
}

std::vector<TaggedFeedUrl> CacheApi::get_subscribed_urls()
{

	auto urls = cache->fetch_feed_urls();
	std::vector<TaggedFeedUrl> url_pairs;

	for (std::string url : urls) {
		url_pairs.push_back({url, {}});
	}

	return url_pairs;
}

bool CacheApi::mark_all_read(const std::string&)
{
	return false;
}


bool CacheApi::mark_article_read(const std::string&, bool)
{
	return false;
}

bool CacheApi::update_article_flags(const std::string&,
	const std::string&, const std::string&)
{
	return false;
}

void CacheApi::add_custom_headers(curl_slist**)
{
}

}
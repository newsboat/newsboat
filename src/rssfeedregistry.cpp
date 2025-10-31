#include "rssfeedregistry.h"

#include "rssfeed.h"
#include "logger.h"

namespace newsboat {

RssFeedRegistry* RssFeedRegistry::get_instance()
{
	static RssFeedRegistry instance;
	return &instance;
}

RssFeedRegistry::RssFeedRegistry()
{
	generations.emplace_back();
	LOG(Level::WARN, "[RssFeedRegistry] Initialized");
}

void RssFeedRegistry::start_new_generation()
{
	generations.emplace_back();
	LOG(Level::WARN, "[RssFeedRegistry] Started a new generation");
}

void RssFeedRegistry::register_shared_ptr(std::shared_ptr<RssFeed> ptr)
{
	std::lock_guard<std::mutex> l(mtx);

	const auto rssurl = ptr->rssurl();
	const auto it = generations.back().rssurl_to_feed_ptrs.find(rssurl);
	if(it == std::end(generations.back().rssurl_to_feed_ptrs))
	{
		generations.back().rssurl_to_feed_ptrs[rssurl] = { std::move(ptr) };
	}
	else
	{
		it->second.emplace_back(std::move(ptr));
	}
}

void RssFeedRegistry::register_rss_feed(const RssFeed* feed)
{
	std::lock_guard<std::mutex> l(mtx);

	const auto rssurl = feed->rssurl();
	++(rssurl_to_object_count[rssurl]);
}

void RssFeedRegistry::unregister_rss_feed(const RssFeed* feed)
{
	std::lock_guard<std::mutex> l(mtx);

	const auto rssurl = feed->rssurl();
	--(rssurl_to_object_count[rssurl]);
}

void RssFeedRegistry::print_report()
{
	std::lock_guard<std::mutex> l(mtx);

	LOG(Level::WARN, "[RssFeedRegistry] %u generations", generations.size());

	int64_t object_count = 0;
	for(const auto& entry : rssurl_to_object_count)
	{
		object_count += entry.second;
	}
	LOG(Level::WARN, "[RssFeedRegistry] currently %3u RssFeed objects are alive", object_count);

	for(size_t i = 0; i < generations.size(); ++i)
	{
		const auto& generation = generations[i];
		int64_t alive_count = 0;
		for(const auto& entry : generation.rssurl_to_feed_ptrs)
		{
			for(const auto& ptr : entry.second)
			{
				if(!ptr.expired())
				{
					++alive_count;
				}
			}
		}
		LOG(Level::WARN, "[RssFeedRegistry] generation %2u contains %3u objects, %u of which are alive", i, generation.rssurl_to_feed_ptrs.size(), alive_count);
	}
}

} // namespace newsboat

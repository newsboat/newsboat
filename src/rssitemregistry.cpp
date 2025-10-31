#include "rssitemregistry.h"

#include "rssitem.h"
#include "logger.h"

namespace newsboat {

RssItemRegistry* RssItemRegistry::get_instance()
{
	static RssItemRegistry instance;
	return &instance;
}

RssItemRegistry::RssItemRegistry()
{
	generations.emplace_back();
	LOG(Level::WARN, "[RssItemRegistry] Initialized");
}

void RssItemRegistry::start_new_generation()
{
	generations.emplace_back();
	LOG(Level::WARN, "[RssItemRegistry] Started a new generation");
}

void RssItemRegistry::register_rss_item(const RssItem* /*item*/)
{
	std::lock_guard<std::mutex> l(mtx);

	++(generations.back().objects_created);
}

void RssItemRegistry::unregister_rss_item(const RssItem* /*item*/)
{
	std::lock_guard<std::mutex> l(mtx);

	++(generations.back().objects_destroyed);
}

void RssItemRegistry::print_report()
{
	std::lock_guard<std::mutex> l(mtx);

	int64_t running_total {0};

	LOG(Level::WARN, "[RssItemRegistry] %u generations", generations.size());

	for(size_t i = 0; i < generations.size(); ++i)
	{
		const auto& generation = generations[i];
		running_total += generation.objects_created - generation.objects_destroyed;
		LOG(Level::WARN, "[RssItemRegistry] generation %2u: %7u created, %7u destroyed, %7u alive", i, generation.objects_created, generation.objects_destroyed, running_total);
	}
}

} // namespace newsboat

#ifndef NEWSBOAT_RSSITEMREGITRY_H_
#define NEWSBOAT_RSSITEMREGITRY_H_

#include <map>
#include <memory>
#include <mutex>
#include <vector>

namespace newsboat {

class RssItem;

class RssItemRegistry {
public:
	static RssItemRegistry* get_instance();

	void start_new_generation();

	void register_rss_item(const RssItem* item);

	void unregister_rss_item(const RssItem* item);

	void print_report();

private:
	RssItemRegistry();

	std::mutex mtx;

	struct Generation {
		uint64_t objects_created {0};
		uint64_t objects_destroyed {0};
	};

	std::vector<Generation> generations;
};

} // namespace newsboat

#endif /* NEWSBOAT_RSSITEMREGITRY_H_ */

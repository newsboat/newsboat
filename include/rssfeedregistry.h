#ifndef NEWSBOAT_RSSFEEDREGITRY_H_
#define NEWSBOAT_RSSFEEDREGITRY_H_

#include <map>
#include <memory>
#include <mutex>
#include <vector>

namespace newsboat {

class RssFeed;

class RssFeedRegistry {
public:
	static RssFeedRegistry* get_instance();

	void start_new_generation();

	void register_shared_ptr(std::shared_ptr<RssFeed> ptr);

	void register_rss_feed(const RssFeed* feed);

	void unregister_rss_feed(const RssFeed* feed);

	void print_report();

private:
	RssFeedRegistry();

	std::mutex mtx;

	struct Generation {
		std::map<std::string, std::vector<std::weak_ptr<RssFeed>>> rssurl_to_feed_ptrs;
	};

	std::vector<Generation> generations;
	std::map<std::string, int64_t> rssurl_to_object_count;
};

} // namespace newsboat

#endif /* NEWSBOAT_RSSFEEDREGITRY_H_ */

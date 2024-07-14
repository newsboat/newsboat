#ifndef NEWSBOAT_CACHE_H_
#define NEWSBOAT_CACHE_H_

#include <memory>
#include <mutex>
#include <sqlite3.h>
#include <unordered_set>

#include "configcontainer.h"
#include "filepath.h"

namespace newsboat {

class RssFeed;
class RssIgnores;
class RssItem;

struct SchemaVersion {
	unsigned int major, minor;

	friend bool operator<(const SchemaVersion& l, const SchemaVersion& r)
	{
		return std::tie(l.major, l.minor) < std::tie(r.major, r.minor);
	}
	friend bool operator>(const SchemaVersion& l, const SchemaVersion& r)
	{
		return r < l;
	}
	friend bool operator<=(const SchemaVersion& l, const SchemaVersion& r)
	{
		return !(l > r);
	}
	friend bool operator>=(const SchemaVersion& l, const SchemaVersion& r)
	{
		return !(l < r);
	}
};

const SchemaVersion unknown_version = {0, 0};

using schema_patches = std::map<SchemaVersion, std::vector<std::string>>;

class Cache {
public:
	Cache(const Filepath& cachefile, ConfigContainer& c);
	~Cache();

	static std::unique_ptr<Cache> in_memory(ConfigContainer& c);

	void externalize_rssfeed(std::shared_ptr<RssFeed> feed,
		bool reset_unread);
	std::shared_ptr<RssFeed> internalize_rssfeed(std::string rssurl,
		RssIgnores* ign);
	void update_rssitem_unread_and_enqueued(std::shared_ptr<RssItem> item,
		const std::string& feedurl);
	void update_rssitem_unread_and_enqueued(RssItem* item,
		const std::string& feedurl);
	/// If requested, removes unreachable data stored in cache.
	/// Returns a list of unreachable feeds.
	std::vector<std::string> cleanup_cache(std::vector<std::shared_ptr<RssFeed>> feeds,
		bool always_clean = false);
	void do_vacuum();
	std::vector<std::shared_ptr<RssItem>> search_for_items(
			const std::string& querystr,
			const std::string& feedurl,
			RssIgnores& ign);
	std::unordered_set<std::string> search_in_items(
		const std::string& querystr,
		const std::unordered_set<std::string>& guids);
	void mark_all_read(const std::string& feedurl = "");
	void mark_all_read(std::shared_ptr<RssFeed> feed);
	void update_rssitem_flags(RssItem* item);
	void fetch_lastmodified(const std::string& uri,
		time_t& t,
		std::string& etag);
	void update_lastmodified(const std::string& uri,
		time_t t,
		const std::string& etag);
	void mark_item_deleted(const std::string& guid, bool b);
	void remove_old_deleted_items(RssFeed* feed);
	void mark_items_read_by_guid(const std::vector<std::string>& guids);
	std::vector<std::string> get_read_item_guids();
	void fetch_descriptions(RssFeed* feed);
	std::string fetch_description(const RssItem& item);

private:
	SchemaVersion get_schema_version();
	void populate_tables();
	void set_pragmas();
	void delete_item_unlocked(const std::shared_ptr<RssItem>& item);
	void clean_old_articles();
	void update_rssitem_unlocked(std::shared_ptr<RssItem> item,
		const std::string& feedurl,
		bool reset_unread);

	std::string prepare_query(const std::string& format);
	template<typename... Args>
	std::string prepare_query(const std::string& format,
		const std::string& arg,
		Args... args);
	template<typename T, typename... Args>
	std::string prepare_query(const std::string& format, const T& arg,
		Args... args);

	void run_sql(const std::string& query,
		int (*callback)(void*, int, char**, char**) = nullptr,
		void* callback_argument = nullptr);
	void run_sql_nothrow(const std::string& query,
		int (*callback)(void*, int, char**, char**) = nullptr,
		void* callback_argument = nullptr);
	void run_sql_impl(const std::string& query,
		int (*callback)(void*, int, char**, char**),
		void* callback_argument,
		bool do_throw);

	sqlite3* db;
	ConfigContainer& cfg;
	std::recursive_mutex mtx;
};

} // namespace newsboat

#endif /* NEWSBOAT_CACHE_H_ */

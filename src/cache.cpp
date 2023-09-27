#include "cache.h"

#include <cassert>
#include <cinttypes>
#include <cstring>
#include <iostream>
#include <sqlite3.h>
#include <sstream>
#include <time.h>

#include "configcontainer.h"
#include "dbexception.h"
#include "logger.h"
#include "matcherexception.h"
#include "rssfeed.h"
#include "rssignores.h"
#include "scopemeasure.h"
#include "strprintf.h"
#include "utils.h"

namespace newsboat {

inline void Cache::run_sql_impl(const std::string& query,
	int (*callback)(void*, int, char**, char**),
	void* callback_argument,
	bool do_throw)
{
	LOG(Level::DEBUG, "running query: %s", query);
	const int rc = sqlite3_exec(
			db, query.c_str(), callback, callback_argument, nullptr);
	if (rc != SQLITE_OK) {
		const std::string message = "query \"%s\" failed: (%d) %s";
		LOG(Level::CRITICAL, message, query, rc, sqlite3_errstr(rc));
		if (do_throw) {
			throw DbException(db);
		}
	}
}

void Cache::run_sql(const std::string& query,
	int (*callback)(void*, int, char**, char**),
	void* callback_argument)
{
	run_sql_impl(query, callback, callback_argument, true);
}

void Cache::run_sql_nothrow(const std::string& query,
	int (*callback)(void*, int, char**, char**),
	void* callback_argument)
{
	run_sql_impl(query, callback, callback_argument, false);
}

struct CbHandler {
	CbHandler()
		: c(-1)
	{
	}
	void set_count(int i)
	{
		c = i;
	}
	int count()
	{
		return c;
	}

private:
	int c;
};

struct HeaderValues {
	time_t lastmodified;
	std::string etag;
};

static int count_callback(void* handler, int argc, char** argv,
	char** /* azColName */)
{
	CbHandler* cbh = static_cast<CbHandler*>(handler);

	if (argc > 0) {
		std::istringstream is(argv[0]);
		int x;
		is >> x;
		cbh->set_count(x);
	}

	return 0;
}

static int single_string_callback(void* handler,
	int argc,
	char** argv,
	char** /* azColName */)
{
	std::string* value = reinterpret_cast<std::string*>(handler);
	if (argc > 0 && argv[0]) {
		*value = argv[0];
	}
	return 0;
}

static int rssfeed_callback(void* myfeed, int argc, char** argv,
	char** /* azColName */)
{
	auto feed = static_cast<RssFeed*>(myfeed);
	// normaly, this shouldn't happen, but we keep the assert()s here
	// nevertheless
	assert(argc == 3);
	assert(argv[0] != nullptr);
	assert(argv[1] != nullptr);
	assert(argv[2] != nullptr);
	feed->set_title(argv[0]);
	feed->set_link(argv[1]);
	feed->set_rtl(strcmp(argv[2], "1") == 0);
	LOG(Level::INFO,
		"rssfeed_callback: title = %s link = %s is_rtl = %s",
		argv[0],
		argv[1],
		argv[2]);
	return 0;
}

static int lastmodified_callback(void* handler,
	int argc,
	char** argv,
	char** /* azColName */)
{
	HeaderValues* result = static_cast<HeaderValues*>(handler);
	assert(argc == 2);
	assert(result != nullptr);
	if (argv[0]) {
		std::istringstream is(argv[0]);
		is >> result->lastmodified;
	} else {
		result->lastmodified = 0;
	}
	if (argv[1]) {
		result->etag = argv[1];
	} else {
		result->etag = "";
	}
	LOG(Level::INFO,
		"lastmodified_callback: lastmodified = %" PRId64 " etag = %s",
		// On GCC, `time_t` is `long int`, which is at least 32 bits long
		// according to the spec. On x86_64, it's actually 64 bits. Thus,
		// casting to int64_t is either a no-op, or an up-cast which are always
		// safe.
		static_cast<int64_t>(result->lastmodified),
		result->etag);
	return 0;
}

static int vectorofstring_callback(void* vp, int argc, char** argv,
	char** /* azColName */)
{
	std::vector<std::string>* vectorptr =
		static_cast<std::vector<std::string>*>(vp);
	assert(argc == 1);
	assert(argv[0] != nullptr);
	vectorptr->push_back(std::string(argv[0]));
	LOG(Level::INFO, "vectorofstring_callback: element = %s", argv[0]);
	return 0;
}

static int rssitem_callback(void* myfeed, int argc, char** argv,
	char** /* azColName */)
{
	auto feed = static_cast<RssFeed*>(myfeed);
	assert(argc == 15);
	auto item = std::make_shared<RssItem>(nullptr);
	item->set_guid(argv[0]);
	item->set_title(argv[1]);
	item->set_author(argv[2]);
	item->set_link(argv[3]);

	std::istringstream is(argv[4]);
	time_t t;
	is >> t;
	item->set_pubDate(t);

	item->set_size(utils::to_u(argv[5]));
	item->set_unread((std::string("1") == argv[6]));

	item->set_feedurl(argv[7]);

	item->set_enclosure_url(argv[8] ? argv[8] : "");
	item->set_enclosure_type(argv[9] ? argv[9] : "");
	item->set_enclosure_description(argv[10] ? argv[10] : "");
	item->set_enclosure_description_mime_type(argv[11] ? argv[11] : "");
	item->set_enqueued((std::string("1") == (argv[12] ? argv[12] : "")));
	item->set_flags(argv[13] ? argv[13] : "");
	item->set_base(argv[14] ? argv[14] : "");

	feed->add_item(item);
	return 0;
}

static int fill_content_callback(void* myfeed,
	int argc,
	char** argv,
	char** /* azColName */)
{
	RssFeed* feed = static_cast<RssFeed*>(myfeed);
	assert(argc == 3);
	if (argv[0]) {
		std::shared_ptr<RssItem> item =
			feed->get_item_by_guid_unlocked(argv[0]);
		item->set_description(argv[1] ? argv[1] : "", argv[2] ? argv[2] : "");
	}
	return 0;
}

static int search_item_callback(void* myfeed,
	int argc,
	char** argv,
	char** /* azColName */)
{
	std::vector<std::shared_ptr<RssItem>>* items =
			static_cast<std::vector<std::shared_ptr<RssItem>>*>(myfeed);
	assert(argc == 15);
	auto item = std::make_shared<RssItem>(nullptr);
	item->set_guid(argv[0]);
	item->set_title(argv[1]);
	item->set_author(argv[2]);
	item->set_link(argv[3]);

	std::istringstream is(argv[4]);
	time_t t;
	is >> t;
	item->set_pubDate(t);

	item->set_size(utils::to_u(argv[5]));
	item->set_unread((std::string("1") == argv[6]));
	item->set_feedurl(argv[7]);

	item->set_enclosure_url(argv[8] ? argv[8] : "");
	item->set_enclosure_type(argv[9] ? argv[9] : "");
	item->set_enclosure_description(argv[10] ? argv[10] : "");
	item->set_enclosure_description_mime_type(argv[11] ? argv[11] : "");
	item->set_enqueued((std::string("1") == argv[12]));
	item->set_flags(argv[13] ? argv[13] : "");
	item->set_base(argv[14] ? argv[14] : "");

	items->push_back(item);
	return 0;
}

static int guid_callback(void* myguids, int argc, char** argv,
	char** /* azColName */)
{
	auto* guids = static_cast<std::unordered_set<std::string>*>(myguids);
	assert(argc == 1);
	guids->emplace(argv[0]);
	return 0;
}

Cache::Cache(const Filepath& cachefile, ConfigContainer* c)
	: cfg(c)
{
	const int error = sqlite3_open(cachefile.to_locale_string().c_str(), &db);
	if (error != SQLITE_OK) {
		LOG(Level::ERROR,
			"couldn't sqlite3_open(%s): error = %d",
			cachefile,
			error);
		throw DbException(db);
	}

	populate_tables();
	set_pragmas();

	clean_old_articles();

	// we need to manually lock all DB operations because SQLite has no
	// explicit support for multithreading.
}

Cache::~Cache()
{
	close_database();
}

void Cache::set_pragmas()
{
	std::lock_guard<std::recursive_mutex> lock(mtx);
	// first, we need to swithc off synchronous writing as it's slow as hell
	run_sql("PRAGMA synchronous = OFF;");

	// then we disable case-sensitive matching for the LIKE operator in
	// SQLite, for search operations
	run_sql("PRAGMA case_sensitive_like=OFF;");
}

static const schema_patches schemaPatches{
	{	{2, 10},
		{
			"CREATE TABLE rss_feed ( " // NOLINT(bugprone-suspicious-missing-comma)
			" rssurl VARCHAR(1024) PRIMARY KEY NOT NULL, "
			" url VARCHAR(1024) NOT NULL, "
			" title VARCHAR(1024) NOT NULL ); ",

			"CREATE TABLE rss_item ( "
			" id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
			" guid VARCHAR(64) NOT NULL, "
			" title VARCHAR(1024) NOT NULL, "
			" author VARCHAR(1024) NOT NULL, "
			" url VARCHAR(1024) NOT NULL, "
			" feedurl VARCHAR(1024) NOT NULL, "
			" pubDate INTEGER NOT NULL, "
			" content VARCHAR(65535) NOT NULL,"
			" unread INTEGER(1) NOT NULL );",

			"CREATE TABLE google_replay ( "
			" id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
			" guid VARCHAR(64) NOT NULL, "
			" state INTEGER NOT NULL, "
			" ts INTEGER NOT NULL );",

			/* we need to do these ALTER TABLE statements because we
			 * need to store additional data for the podcast support
			 */
			"ALTER TABLE rss_item ADD enclosure_url VARCHAR(1024);",

			"ALTER TABLE rss_item ADD enclosure_type "
			"VARCHAR(1024);",

			"ALTER TABLE rss_item ADD enqueued INTEGER(1) NOT NULL "
			"DEFAULT 0;",

			"ALTER TABLE rss_item ADD flags VARCHAR(52);",

			/* create indexes to speed up certain queries */
			"CREATE INDEX IF NOT EXISTS idx_rssurl ON "
			"rss_feed(rssurl);",

			"CREATE INDEX IF NOT EXISTS idx_guid ON "
			"rss_item(guid);",

			"CREATE INDEX IF NOT EXISTS idx_feedurl ON "
			"rss_item(feedurl);",
			/* we analyse the indices for better statistics */
			"ANALYZE;",

			"ALTER TABLE rss_feed ADD lastmodified INTEGER(11) NOT "
			"NULL "
			"DEFAULT 0;",

			"CREATE INDEX IF NOT EXISTS idx_lastmodified ON "
			"rss_feed(lastmodified);",

			"ALTER TABLE rss_item ADD deleted INTEGER(1) NOT NULL "
			"DEFAULT "
			"0;",

			"CREATE INDEX IF NOT EXISTS idx_deleted ON "
			"rss_item(deleted);",

			"ALTER TABLE rss_feed ADD is_rtl INTEGER(1) NOT NULL "
			"DEFAULT "
			"0;",

			"ALTER TABLE rss_feed ADD etag VARCHAR(128) NOT NULL "
			"DEFAULT "
			"\"\";",

			"ALTER TABLE rss_item ADD base VARCHAR(128) NOT NULL "
			"DEFAULT "
			"\"\";",
		}
	},
	{	{2, 11},
		{
			"CREATE TABLE metadata ( "
			" db_schema_version_major INTEGER NOT NULL, "
			" db_schema_version_minor INTEGER NOT NULL );",

			"INSERT INTO metadata VALUES ( 2, 11 );"
		}
	},
	{	{2, 22},
		{
			"ALTER TABLE rss_item ADD COLUMN content_mime_type VARCHAR(255) NOT NULL DEFAULT \"\";"
		}
	},
	{	{2, 33},
		{
			"ALTER TABLE rss_item ADD COLUMN enclosure_description VARCHAR(1024) NOT NULL DEFAULT \"\";",
			"ALTER TABLE rss_item ADD COLUMN enclosure_description_mime_type VARCHAR(128) NOT NULL DEFAULT \"\";",
		}
	},

	// Note: schema changes should use the version number of the release that introduced them.
};

void Cache::populate_tables()
{
	std::lock_guard<std::recursive_mutex> lock(mtx);
	const SchemaVersion version = get_schema_version();
	LOG(Level::INFO,
		"Cache::populate_tables: DB schema version %u.%u",
		version.major,
		version.minor);

	if (version.major > utils::newsboat_major_version()) {
		const std::string msg =
			"Database schema isn't supported because it's too new";
		LOG(Level::ERROR, msg);
		throw std::runtime_error(msg);
	}

	auto patches_it = schemaPatches.cbegin();

	// rewind to the first patch that should be applied
	while (patches_it != schemaPatches.cend() &&
		patches_it->first <= version) {
		++patches_it;
	}

	for (; patches_it != schemaPatches.cend(); ++patches_it) {
		const SchemaVersion patch_version = patches_it->first;
		LOG(Level::INFO,
			"Cache::populate_tables: applying DB schema patches "
			"for version %u.%u",
			patch_version.major,
			patch_version.minor);

		std::string update_metadata_query = "UPDATE metadata SET db_schema_version_major = " +
			std::to_string(patch_version.major) + ", db_schema_version_minor = " +
			std::to_string(patch_version.minor) + ";";

		if (patch_version > SchemaVersion{2, 11}) {
			run_sql_nothrow(update_metadata_query);
		}

		for (const auto& query : patches_it->second) {
			run_sql_nothrow(query);
		}
	}
}

void Cache::fetch_lastmodified(const std::string& feedurl,
	time_t& t,
	std::string& etag)
{
	std::lock_guard<std::recursive_mutex> lock(mtx);
	std::string query = prepare_query(
			"SELECT lastmodified, etag FROM rss_feed WHERE rssurl = '%q';",
			feedurl);
	HeaderValues result = {0, ""};
	run_sql(query, lastmodified_callback, &result);
	t = result.lastmodified;
	etag = result.etag;
	LOG(Level::DEBUG,
		"Cache::fetch_lastmodified: t = %" PRId64 " etag = %s",
		// On GCC, `time_t` is `long int`, which is at least 32 bits. On
		// x86_64, it's 64 bits. Thus, this cast is either a no-op, or an
		// up-cast which is always safe.
		static_cast<int64_t>(t),
		etag);
}

void Cache::update_lastmodified(const std::string& feedurl,
	time_t t,
	const std::string& etag)
{
	if (t == 0 && etag.length() == 0) {
		LOG(Level::INFO,
			"Cache::update_lastmodified: both time and etag are "
			"empty, not updating anything");
		return;
	}

	run_sql(prepare_query("INSERT OR IGNORE INTO rss_feed (rssurl, url, title) VALUES ('%q', '', '')",
			feedurl));

	std::lock_guard<std::recursive_mutex> lock(mtx);
	std::string query = "UPDATE rss_feed SET ";
	if (t > 0) {
		query.append(prepare_query("lastmodified = '%d'", t));
	}
	if (etag.length() > 0) {
		query.append(prepare_query("%c etag = %s",
				(t > 0 ? ',' : ' '),
				prepare_query("'%q'", etag)));
	}
	query.append(" WHERE rssurl = ");
	query.append(prepare_query("'%q'", feedurl));
	run_sql_nothrow(query);
}

void Cache::mark_item_deleted(const std::string& guid, bool b)
{
	std::lock_guard<std::recursive_mutex> lock(mtx);
	std::string query = prepare_query(
			"UPDATE rss_item SET deleted = %u WHERE guid = '%q'",
			b ? 1 : 0,
			guid);
	run_sql_nothrow(query);
}

// this function writes an RssFeed including all RssItems to the database
void Cache::externalize_rssfeed(RssFeed& feed,
	bool reset_unread)
{
	ScopeMeasure m1("Cache::externalize_feed");
	if (feed.is_query_feed()) {
		return;
	}

	std::lock_guard<std::recursive_mutex> lock(mtx);
	std::lock_guard<std::mutex> feedlock(feed.item_mutex);
	// scope_transaction dbtrans(db);

	CbHandler count_cbh;
	auto query = prepare_query(
			"SELECT count(*) FROM rss_feed WHERE rssurl = '%q';",
			feed.rssurl());
	run_sql(query, count_callback, &count_cbh);

	const int count = count_cbh.count();
	LOG(Level::DEBUG,
		"Cache::externalize_rss_feed: rss_feeds with rssurl = '%s': "
		"found "
		"%d",
		feed.rssurl(),
		count);
	if (count > 0) {
		const std::string updatequery = prepare_query(
				"UPDATE rss_feed "
				"SET title = '%q', url = '%q', is_rtl = %u "
				"WHERE rssurl = '%q';",
				feed.title_raw(),
				feed.link(),
				feed.is_rtl() ? 1 : 0,
				feed.rssurl());
		run_sql(updatequery);
	} else {
		const std::string insertquery = prepare_query(
				"INSERT INTO rss_feed (rssurl, url, title, is_rtl) "
				"VALUES ( '%q', '%q', '%q', %u );",
				feed.rssurl(),
				feed.link(),
				feed.title_raw(),
				feed.is_rtl() ? 1 : 0);
		run_sql(insertquery);
	}

	const unsigned int max_items = cfg->get_configvalue_as_int("max-items");

	LOG(Level::INFO,
		"Cache::externalize_feed: max_items = %u "
		"feed.total_item_count() = "
		"%u",
		max_items,
		feed.total_item_count());

	if (max_items > 0 && feed.total_item_count() > max_items) {
		feed.erase_items(
			feed.items().begin() + max_items, feed.items().end());
	}

	const unsigned int days = cfg->get_configvalue_as_int("keep-articles-days");
	const time_t old_time = time(nullptr) - days * 24 * 60 * 60;

	// the reverse iterator is there for the sorting foo below (think about
	// it)
	for (auto it = feed.items().rbegin(); it != feed.items().rend();
		++it) {
		RssItem& item = **it;
		if (days == 0 || item.pubDate_timestamp() >= old_time)
			update_rssitem_unlocked(
				item, feed.rssurl(), reset_unread);
	}
}

// this function reads an RssFeed including all of its RssItems.
// the feed parameter needs to have the rssurl member set.
std::shared_ptr<RssFeed> Cache::internalize_rssfeed(std::string rssurl,
	RssIgnores* ign)
{
	ScopeMeasure m1("Cache::internalize_rssfeed");

	std::shared_ptr<RssFeed> feed(new RssFeed(this, rssurl));

	if (utils::is_query_url(rssurl)) {
		return feed;
	}

	std::lock_guard<std::recursive_mutex> lock(mtx);
	std::lock_guard<std::mutex> feedlock(feed->item_mutex);

	/* first, we check whether the feed is there at all */
	std::string query = prepare_query(
			"SELECT count(*) FROM rss_feed WHERE rssurl = '%q';", rssurl);
	CbHandler count_cbh;
	run_sql(query, count_callback, &count_cbh);

	if (count_cbh.count() == 0) {
		return feed;
	}

	/* then we first read the feed from the database */
	query = prepare_query(
			"SELECT title, url, is_rtl FROM rss_feed WHERE rssurl = '%q';",
			rssurl);
	run_sql(query, rssfeed_callback, feed.get());

	/* ...and then the associated items */
	query = prepare_query(
			"SELECT guid, title, author, url, pubDate, length(content), "
			"unread, "
			"feedurl, enclosure_url, enclosure_type, enclosure_description, enclosure_description_mime_type, "
			"enqueued, flags, base "
			"FROM rss_item "
			"WHERE feedurl = '%q' "
			"AND deleted = 0 "
			"ORDER BY pubDate DESC, id DESC;",
			rssurl);
	run_sql(query, rssitem_callback, feed.get());

	auto feed_weak_ptr = std::weak_ptr<RssFeed>(feed);
	for (const auto& item : feed->items()) {
		item->set_cache(this);
		item->set_feedptr(feed_weak_ptr);
		item->set_feedurl(feed->rssurl());
	}

	if (ign != nullptr) {
		auto& items = feed->items();
		items.erase(
			std::remove_if(
				items.begin(),
				items.end(),
		[&](std::shared_ptr<RssItem> item) -> bool {
			try
			{
				return ign->matches(item.get());
			} catch (const MatcherException& ex)
			{
				LOG(Level::DEBUG,
					"oops, Matcher exception: %s",
					ex.what());
				return false;
			}
		}),
		items.end());
	}

	const unsigned int max_items = cfg->get_configvalue_as_int("max-items");

	if (max_items > 0 && feed->total_item_count() > max_items) {
		std::vector<std::shared_ptr<RssItem>> flagged_items;
		for (unsigned int j = max_items; j < feed->total_item_count();
			++j) {
			if (feed->items()[j]->flags().length() == 0) {
				delete_item_unlocked(*feed->items()[j]);
			} else {
				flagged_items.push_back(feed->items()[j]);
			}
		}

		auto it = feed->items().begin() + max_items;
		feed->erase_items(
			it, feed->items().end()); // delete old entries

		// if some flagged articles were saved, append them
		feed->add_items(flagged_items);
	}
	feed->sort_unlocked(cfg->get_article_sort_strategy());
	return feed;
}

std::vector<std::shared_ptr<RssItem>> Cache::search_for_items(
		const std::string& querystr, const std::string& feedurl,
		RssIgnores& ign)
{
	assert(!utils::is_query_url(feedurl));
	std::string query;
	std::vector<std::shared_ptr<RssItem>> items;

	std::lock_guard<std::recursive_mutex> lock(mtx);
	if (feedurl.length() > 0) {
		query = prepare_query(
				"SELECT guid, title, author, url, pubDate, "
				"length(content), "
				"unread, feedurl, enclosure_url, enclosure_type, "
				"enclosure_description, enclosure_description_mime_type, "
				"enqueued, flags, base "
				"FROM rss_item "
				"WHERE (title LIKE '%%%q%%' OR content LIKE '%%%q%%') "
				"AND feedurl = '%q' "
				"AND deleted = 0 "
				"ORDER BY pubDate DESC, id DESC;",
				querystr,
				querystr,
				feedurl);
	} else {
		query = prepare_query(
				"SELECT guid, title, author, url, pubDate, "
				"length(content), "
				"unread, feedurl, enclosure_url, enclosure_type, "
				"enclosure_description, enclosure_description_mime_type, "
				"enqueued, flags, base "
				"FROM rss_item "
				"WHERE (title LIKE '%%%q%%' OR content LIKE '%%%q%%') "
				"AND deleted = 0 "
				"ORDER BY pubDate DESC,  id DESC;",
				querystr,
				querystr);
	}

	run_sql(query, search_item_callback, &items);
	for (const auto& item : items) {
		item->set_cache(this);
	}
	items.erase(
		std::remove_if(
			items.begin(),
			items.end(),
	[&](std::shared_ptr<RssItem> item) -> bool {
		try
		{
			return ign.matches(item.get());
		} catch (const MatcherException& ex)
		{
			LOG(Level::DEBUG,
				"Cache::search_for_items: oops, Matcher exception: %s",
				ex.what());
			return false;
		}
	}),
	items.end());

	return items;
}

std::unordered_set<std::string> Cache::search_in_items(
	const std::string& querystr,
	const std::unordered_set<std::string>& guids)
{
	std::lock_guard<std::recursive_mutex> lock(mtx);

	std::string list = "(";
	for (const auto& guid : guids) {
		list.append(prepare_query("%Q, ", guid));
	}
	list.append("'')");

	std::string query = prepare_query(
			"SELECT guid "
			"FROM rss_item "
			"WHERE (title LIKE '%%%q%%' OR content LIKE '%%%q%%') "
			"AND guid IN %s;",
			querystr,
			querystr,
			list);

	std::unordered_set<std::string> items;
	run_sql(query, guid_callback, &items);
	return items;
}

void Cache::delete_item_unlocked(const RssItem& item)
{
	const std::string query = prepare_query(
			"DELETE FROM rss_item WHERE guid = '%q';", item.guid());
	run_sql(query);
}

void Cache::do_vacuum()
{
	std::lock_guard<std::recursive_mutex> lock(mtx);
	run_sql("VACUUM;");
}

std::vector<std::string> Cache::cleanup_cache(std::vector<std::shared_ptr<RssFeed>> feeds,
	bool always_clean)
{
	std::lock_guard<std::recursive_mutex> lock(mtx);

	std::vector<std::string> unreachable_feeds{};
	std::string list = "(";

	for (const auto& feed : feeds) {
		const auto name = prepare_query("'%q'", feed->rssurl());
		list.append(name);
		list.append(", ");
	}
	list.append("'')");

	/*
	 * cache cleanup means that all entries in both the RssFeed and
	 * RssItem tables that are associated with an RSS feed URL that is not
	 * contained in the current configuration are deleted. Such entries are
	 * the result when a user deletes one or more lines in the urls
	 * configuration file. We then assume that the user isn't interested
	 * anymore in reading this feed, and delete all associated entries
	 * because they would be non-accessible.
	 *
	 * The behaviour whether the cleanup is done or not is configurable via
	 * the configuration file.
	 */
	if (always_clean || cfg->get_configvalue_as_bool("cleanup-on-quit")) {
		LOG(Level::DEBUG, "Cache::cleanup_cache: cleaning up cache...");

		std::string cleanup_rss_feeds_statement(
			"DELETE FROM rss_feed WHERE rssurl NOT IN ");
		cleanup_rss_feeds_statement.append(list);
		cleanup_rss_feeds_statement.push_back(';');

		std::string cleanup_rss_items_statement(
			"DELETE FROM rss_item WHERE feedurl NOT IN ");
		cleanup_rss_items_statement.append(list);
		cleanup_rss_items_statement.push_back(';');

		std::string cleanup_read_items_statement(
			"UPDATE rss_item SET deleted = 1 WHERE unread = 0");

		run_sql(cleanup_rss_feeds_statement);
		run_sql(cleanup_rss_items_statement);
		if (cfg->get_configvalue_as_bool(
				"delete-read-articles-on-quit")) {
			run_sql(cleanup_read_items_statement);
		}
	} else {
		LOG(Level::DEBUG,
			"Cache::cleanup_cache: NOT cleaning up cache...");

		std::string query = (
				"SELECT DISTINCT rss "
				"FROM ("
				"SELECT feedurl AS rss FROM rss_item "
				"UNION ALL "
				"SELECT rssurl FROM rss_feed"
				") "
				"WHERE rss NOT IN "
			);
		query.append(list);
		query.push_back(';');

		run_sql(query, vectorofstring_callback, &unreachable_feeds);
	}

	// Ensure that no other operations can occur after the cache cleanup
	close_database();

	return unreachable_feeds;
}

void Cache::update_rssitem_unlocked(RssItem& item,
	const std::string& feedurl,
	bool reset_unread)
{
	std::string query = prepare_query(
			"SELECT count(*) FROM rss_item WHERE guid = '%q';",
			item.guid());
	CbHandler count_cbh;
	run_sql(query, count_callback, &count_cbh);
	const auto description = item.description();
	if (count_cbh.count() > 0) {
		if (reset_unread) {
			std::string content;
			query = prepare_query(
					"SELECT content FROM rss_item WHERE guid = "
					"'%q';",
					item.guid());
			run_sql(query, single_string_callback, &content);
			if (content != description.text) {
				LOG(Level::DEBUG,
					"Cache::update_rssitem_unlocked: '%s' "
					"is "
					"different from '%s'",
					content,
					description.text);
				query = prepare_query(
						"UPDATE rss_item SET unread = 1 WHERE "
						"guid = '%q';",
						item.guid());
				run_sql(query);
			}
		}
		std::string update;
		if (item.override_unread()) {
			update = prepare_query(
					"UPDATE rss_item "
					"SET title = '%q', author = '%q', url = '%q', "
					"feedurl = '%q', "
					"content = '%q', content_mime_type = '%q', enclosure_url = '%q', "
					"enclosure_type = '%q', enclosure_description = '%q', "
					"enclosure_description_mime_type = '%q', base = '%q', unread = "
					"'%d' "
					"WHERE guid = '%q'",
					item.title(),
					item.author(),
					item.link(),
					feedurl,
					description.text,
					description.mime,
					item.enclosure_url(),
					item.enclosure_type(),
					item.enclosure_description(),
					item.enclosure_description_mime_type(),
					item.get_base(),
					(item.unread() ? 1 : 0),
					item.guid());
		} else {
			update = prepare_query(
					"UPDATE rss_item "
					"SET title = '%q', author = '%q', url = '%q', "
					"feedurl = '%q', "
					"content = '%q', content_mime_type = '%q', enclosure_url = '%q', "
					"enclosure_type = '%q', enclosure_description = '%q', "
					"enclosure_description_mime_type = '%q', base = '%q' "
					"WHERE guid = '%q'",
					item.title(),
					item.author(),
					item.link(),
					feedurl,
					description.text,
					description.mime,
					item.enclosure_url(),
					item.enclosure_type(),
					item.enclosure_description(),
					item.enclosure_description_mime_type(),
					item.get_base(),
					item.guid());
		}
		run_sql(update);
	} else {
		std::int64_t pubTimestamp = item.pubDate_timestamp();
		std::string insert = prepare_query(
				"INSERT INTO rss_item (guid, title, author, url, "
				"feedurl, "
				"pubDate, content, content_mime_type, unread, enclosure_url, "
				"enclosure_type, enclosure_description, enclosure_description_mime_type, "
				"enqueued, base) "
				"VALUES "
				"('%q','%q','%q','%q','%q','%" PRId64 "','%q','%q','%d','%q','%q','%q','%q',%d, '%q')",
				item.guid(),
				item.title(),
				item.author(),
				item.link(),
				feedurl,
				pubTimestamp,
				description.text,
				description.mime,
				(item.unread() ? 1 : 0),
				item.enclosure_url(),
				item.enclosure_type(),
				item.enclosure_description(),
				item.enclosure_description_mime_type(),
				item.enqueued() ? 1 : 0,
				item.get_base());
		run_sql(insert);
	}
}

void Cache::mark_all_read(RssFeed& feed)
{
	std::lock_guard<std::recursive_mutex> lock(mtx);
	std::lock_guard<std::mutex> itemlock(feed.item_mutex);
	std::string query =
		"UPDATE rss_item SET unread = '0' WHERE unread != '0' AND guid "
		"IN (";

	for (const auto& item : feed.items()) {
		query.append(prepare_query("'%q',", item->guid()));
	}
	query.append("'');");

	run_sql(query);
}

/* this function marks all RssItems (optionally of a certain feed url) as read
 */
void Cache::mark_all_read(const std::string& feedurl)
{
	std::lock_guard<std::recursive_mutex> lock(mtx);

	std::string query;
	if (feedurl.length() > 0) {
		query = prepare_query(
				"UPDATE rss_item "
				"SET unread = '0' "
				"WHERE unread != '0' "
				"AND feedurl = '%q';",
				feedurl);
	} else {
		query = prepare_query(
				"UPDATE rss_item "
				"SET unread = '0' "
				"WHERE unread != '0';");
	}
	run_sql(query);
}

void Cache::update_rssitem_unread_and_enqueued(RssItem& item,
	const std::string& /* feedurl */)
{
	std::lock_guard<std::recursive_mutex> lock(mtx);

	const auto query = prepare_query(
			"UPDATE rss_item "
			"SET unread = '%d', enqueued = '%d' "
			"WHERE guid = '%q'",
			item.unread() ? 1 : 0,
			item.enqueued() ? 1 : 0,
			item.guid());
	run_sql(query);
}

/* helper function to wrap std::string around the sqlite3_*mprintf function */
std::string Cache::prepare_query(const std::string& format)
{
	return format;
}

template<typename... Args>
std::string Cache::prepare_query(const std::string& format,
	const std::string& argument,
	Args... args)
{
	return prepare_query(format, argument.c_str(), args...);
}

template<typename T, typename... Args>
std::string Cache::prepare_query(const std::string& format, const T& argument,
	Args... args)
{
	std::string local_format, remaining_format;
	std::tie(local_format, remaining_format) =
		strprintf::split_format(format);

	char* piece = sqlite3_mprintf(local_format.c_str(), argument);
	std::string result;
	if (piece) {
		result = piece;
		sqlite3_free(piece);
	}

	return result + prepare_query(remaining_format, args...);
}

void Cache::update_rssitem_flags(RssItem* item)
{
	std::lock_guard<std::recursive_mutex> lock(mtx);

	const std::string update = prepare_query(
			"UPDATE rss_item SET flags = '%q' WHERE guid = '%q';",
			item->flags(),
			item->guid());

	run_sql(update);
}

void Cache::remove_old_deleted_items(RssFeed* feed)
{
	ScopeMeasure m1("Cache::remove_old_deleted_items");

	std::lock_guard<std::recursive_mutex> cache_lock(mtx);
	std::lock_guard<std::mutex> feed_lock(feed->item_mutex);

	std::vector<std::string> guids;
	for (const auto& item : feed->items()) {
		guids.push_back(item->guid());
	}

	if (guids.empty()) {
		LOG(Level::DEBUG,
			"Cache::remove_old_deleted_items: not cleaning up "
			"anything because last reload brought no new items "
			"(detected no changes)");
		return;
	}
	std::string guidset = "(";
	for (const auto& guid : guids) {
		guidset.append(prepare_query("'%q', ", guid));
	}
	guidset.append("'')");
	const std::string query = prepare_query(
			"DELETE FROM rss_item "
			"WHERE feedurl = '%q' "
			"AND deleted = 1 "
			"AND guid NOT IN %s;",
			feed->rssurl(),
			guidset);
	run_sql(query);
}

void Cache::mark_items_read_by_guid(const std::vector<std::string>& guids)
{
	ScopeMeasure m1("Cache::mark_items_read_by_guid");
	std::lock_guard<std::recursive_mutex> lock(mtx);
	std::string guidset("(");
	for (const auto& guid : guids) {
		guidset.append(prepare_query("'%q', ", guid));
	}
	guidset.append("'')");

	const std::string updatequery = prepare_query(
			"UPDATE rss_item SET unread = 0 WHERE unread = 1 AND guid IN "
			"%s;",
			guidset);

	run_sql(updatequery);
}

std::vector<std::string> Cache::get_read_item_guids()
{
	std::vector<std::string> guids;
	const std::string query = "SELECT guid FROM rss_item WHERE unread = 0;";

	std::lock_guard<std::recursive_mutex> lock(mtx);
	run_sql(query, vectorofstring_callback, &guids);

	return guids;
}

void Cache::clean_old_articles()
{
	std::lock_guard<std::recursive_mutex> lock(mtx);

	const unsigned int days = cfg->get_configvalue_as_int("keep-articles-days");
	if (days > 0) {
		const time_t old_date = time(nullptr) - days * 24 * 60 * 60;

		const std::string query(prepare_query(
				"DELETE FROM rss_item WHERE pubDate < %d", old_date));
		LOG(Level::DEBUG,
			"Cache::clean_old_articles: about to delete articles "
			"with a pubDate older than %" PRId64,
			// On GCC, `time_t` is `long int`, which is at least 32 bits long
			// according to the spec. On x86_64, it's actually 64 bits. Thus,
			// casting to int64_t is either a no-op, or an up-cast which are
			// always safe.
			static_cast<int64_t>(old_date));
		run_sql(query);
	} else {
		LOG(Level::DEBUG,
			"Cache::clean_old_articles, days == 0, not cleaning up "
			"anything");
	}
}

void Cache::fetch_descriptions(RssFeed* feed)
{
	std::lock_guard<std::recursive_mutex> lock(mtx);
	std::vector<std::string> guids;
	for (const auto& item : feed->items()) {
		guids.push_back(prepare_query("'%q'", item->guid()));
	}
	const std::string in_clause = utils::join(guids, ", ");

	const std::string query = prepare_query(
			"SELECT guid, content, content_mime_type FROM rss_item WHERE guid IN (%s);",
			in_clause);

	run_sql(query, fill_content_callback, feed);
}

std::string Cache::fetch_description(const RssItem& item)
{
	std::lock_guard<std::recursive_mutex> lock(mtx);
	const std::string in_clause = prepare_query("'%q'", item.guid());

	const std::string query = prepare_query(
			"SELECT content FROM rss_item WHERE guid = %s;",
			in_clause);

	std::string description;
	auto store_description = [](void* d, int, char** argv, char**) -> int {
		auto& desc = *static_cast<std::string*>(d);
		desc = argv[0] ? argv[0] : "";
		return 0;
	};

	run_sql(query, store_description, &description);
	return description;
}

SchemaVersion Cache::get_schema_version()
{
	sqlite3_stmt* stmt{};
	SchemaVersion result;

	std::lock_guard<std::recursive_mutex> lock(mtx);
	int rc = sqlite3_prepare_v2(db,
			"SELECT db_schema_version_major, db_schema_version_minor "
			"FROM metadata",
			-1,
			&stmt,
			nullptr);

	if (rc != SQLITE_OK) {
		// I'm pretty sure the query above is correct, and the only way
		// it can fail is when metadata table is not present in the DB.
		// That means we're dealing with an empty cache file, or one
		// that was created by an older version of Newsboat.
		result = unknown_version;
	} else {
		rc = sqlite3_step(stmt);
		if (rc != SQLITE_ROW) {
			// table is empty. Technically, this is impossible, but
			// is easy enough to fix - just re-create the db
			result = unknown_version;
		} else {
			// row is available, let's grab it!
			result.major = sqlite3_column_int(stmt, 0);
			result.minor = sqlite3_column_int(stmt, 1);

			assert(sqlite3_step(stmt) == SQLITE_DONE);
		}
	}

	sqlite3_finalize(stmt);
	return result;
}

void Cache::close_database()
{
	if (db != nullptr) {
		sqlite3_close(db);
		db = nullptr;
	}
}

} // namespace newsboat

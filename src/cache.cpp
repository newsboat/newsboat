#include "cache.h"

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sqlite3.h>
#include <sstream>
#include <time.h>

#include "config.h"
#include "configcontainer.h"
#include "controller.h"
#include "exceptions.h"
#include "logger.h"
#include "rss.h"
#include "strprintf.h"
#include "utils.h"

namespace newsboat {

inline void Cache::run_sql_impl(const std::string& query,
	int (*callback)(void*, int, char**, char**),
	void* callback_argument,
	bool do_throw)
{
	LOG(Level::DEBUG, "running query: %s", query);
	int rc = sqlite3_exec(
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

struct cb_handler {
	cb_handler()
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

struct header_values {
	time_t lastmodified;
	std::string etag;
};

static int
count_callback(void* handler, int argc, char** argv, char** /* azColName */)
{
	cb_handler* cbh = static_cast<cb_handler*>(handler);

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

static int
rssfeed_callback(void* myfeed, int argc, char** argv, char** /* azColName */)
{
	std::shared_ptr<RssFeed>* feed =
		static_cast<std::shared_ptr<RssFeed>*>(myfeed);
	// normaly, this shouldn't happen, but we keep the assert()s here
	// nevertheless
	assert(argc == 3);
	assert(argv[0] != nullptr);
	assert(argv[1] != nullptr);
	assert(argv[2] != nullptr);
	(*feed)->set_title(argv[0]);
	(*feed)->set_link(argv[1]);
	(*feed)->set_rtl(strcmp(argv[2], "1") == 0);
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
	header_values* result = static_cast<header_values*>(handler);
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
		"lastmodified_callback: lastmodified = %d etag = %s",
		result->lastmodified,
		result->etag);
	return 0;
}

static int
vectorofstring_callback(void* vp, int argc, char** argv, char** /* azColName */)
{
	std::vector<std::string>* vectorptr =
		static_cast<std::vector<std::string>*>(vp);
	assert(argc == 1);
	assert(argv[0] != nullptr);
	vectorptr->push_back(std::string(argv[0]));
	LOG(Level::INFO, "vectorofstring_callback: element = %s", argv[0]);
	return 0;
}

static int
rssitem_callback(void* myfeed, int argc, char** argv, char** /* azColName */)
{
	std::shared_ptr<RssFeed>* feed =
		static_cast<std::shared_ptr<RssFeed>*>(myfeed);
	assert(argc == 13);
	std::shared_ptr<RssItem> item(new RssItem(nullptr));
	item->set_guid(argv[0]);
	item->set_title(argv[1]);
	item->set_author(argv[2]);
	item->set_link(argv[3]);

	std::istringstream is(argv[4]);
	time_t t;
	is >> t;
	item->set_pubDate(t);

	item->set_size(Utils::to_u(argv[5]));
	item->set_unread((std::string("1") == argv[6]));

	item->set_feedurl(argv[7]);

	item->set_enclosure_url(argv[8] ? argv[8] : "");
	item->set_enclosure_type(argv[9] ? argv[9] : "");
	item->set_enqueued((std::string("1") == (argv[10] ? argv[10] : "")));
	item->set_flags(argv[11] ? argv[11] : "");
	item->set_base(argv[12] ? argv[12] : "");

	//(*feed)->items().push_back(item);
	(*feed)->add_item(item);
	return 0;
}

static int fill_content_callback(void* myfeed,
	int argc,
	char** argv,
	char** /* azColName */)
{
	RssFeed* feed = static_cast<RssFeed*>(myfeed);
	assert(argc == 2);
	if (argv[0]) {
		std::shared_ptr<RssItem> item =
			feed->get_item_by_guid_unlocked(argv[0]);
		item->set_description(argv[1] ? argv[1] : "");
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
	assert(argc == 13);
	std::shared_ptr<RssItem> item(new RssItem(nullptr));
	item->set_guid(argv[0]);
	item->set_title(argv[1]);
	item->set_author(argv[2]);
	item->set_link(argv[3]);

	std::istringstream is(argv[4]);
	time_t t;
	is >> t;
	item->set_pubDate(t);

	item->set_size(Utils::to_u(argv[5]));
	item->set_unread((std::string("1") == argv[6]));
	item->set_feedurl(argv[7]);

	item->set_enclosure_url(argv[8] ? argv[8] : "");
	item->set_enclosure_type(argv[9] ? argv[9] : "");
	item->set_enqueued((std::string("1") == argv[10]));
	item->set_flags(argv[11] ? argv[11] : "");
	item->set_base(argv[12] ? argv[12] : "");

	items->push_back(item);
	return 0;
}

static int
guid_callback(void* myguids, int argc, char** argv, char** /* azColName */)
{
	auto* guids = static_cast<std::unordered_set<std::string>*>(myguids);
	assert(argc == 1);
	guids->emplace(argv[0]);
	return 0;
}

Cache::Cache(const std::string& Cachefile, ConfigContainer* c)
	: db(0)
	, cfg(c)
{
	int error = sqlite3_open(Cachefile.c_str(), &db);
	if (error != SQLITE_OK) {
		LOG(Level::ERROR,
			"couldn't sqlite3_open(%s): error = %d",
			Cachefile,
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
	sqlite3_close(db);
}

void Cache::set_pragmas()
{
	// first, we need to swithc off synchronous writing as it's slow as hell
	run_sql("PRAGMA synchronous = OFF;");

	// then we disable case-sensitive matching for the LIKE operator in
	// SQLite, for search operations
	run_sql("PRAGMA case_sensitive_like=OFF;");
}

static const schema_patches schemaPatches{
	{{2, 10},
		{
			"CREATE TABLE RssFeed ( "
			" rssurl VARCHAR(1024) PRIMARY KEY NOT NULL, "
			" url VARCHAR(1024) NOT NULL, "
			" title VARCHAR(1024) NOT NULL ); ",

			"CREATE TABLE RssItem ( "
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
			"ALTER TABLE RssItem ADD enclosure_url VARCHAR(1024);",

			"ALTER TABLE RssItem ADD enclosure_type "
			"VARCHAR(1024);",

			"ALTER TABLE RssItem ADD enqueued INTEGER(1) NOT NULL "
			"DEFAULT 0;",

			"ALTER TABLE RssItem ADD flags VARCHAR(52);",

			/* create indexes to speed up certain queries */
			"CREATE INDEX IF NOT EXISTS idx_rssurl ON "
			"RssFeed(rssurl);",

			"CREATE INDEX IF NOT EXISTS idx_guid ON "
			"RssItem(guid);",

			"CREATE INDEX IF NOT EXISTS idx_feedurl ON "
			"RssItem(feedurl);",
			/* we analyse the indices for better statistics */
			"ANALYZE;",

			"ALTER TABLE RssFeed ADD lastmodified INTEGER(11) NOT "
			"NULL "
			"DEFAULT 0;",

			"CREATE INDEX IF NOT EXISTS idx_lastmodified ON "
			"RssFeed(lastmodified);",

			"ALTER TABLE RssItem ADD deleted INTEGER(1) NOT NULL "
			"DEFAULT "
			"0;",

			"CREATE INDEX IF NOT EXISTS idx_deleted ON "
			"RssItem(deleted);",

			"ALTER TABLE RssFeed ADD is_rtl INTEGER(1) NOT NULL "
			"DEFAULT "
			"0;",

			"ALTER TABLE RssFeed ADD etag VARCHAR(128) NOT NULL "
			"DEFAULT "
			"\"\";",

			"ALTER TABLE RssItem ADD base VARCHAR(128) NOT NULL "
			"DEFAULT "
			"\"\";",
		}},
	{{2, 11},
		{"CREATE TABLE metadata ( "
		 " db_schema_version_major INTEGER NOT NULL, "
		 " db_schema_version_minor INTEGER NOT NULL );"

		 "INSERT INTO metadata VALUES ( 2, 11 );"}}};

void Cache::populate_tables()
{
	const schema_version version = get_schema_version();
	LOG(Level::INFO,
		"Cache::populate_tables: DB schema version %u.%u",
		version.major,
		version.minor);

	if (version.major > NEWSBOAT_VERSION_MAJOR) {
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
		const schema_version patch_version = patches_it->first;
		LOG(Level::INFO,
			"Cache::populate_tables: applying DB schema patches "
			"for version %u.%u",
			patch_version.major,
			patch_version.minor);
		for (const auto& query : patches_it->second) {
			run_sql_nothrow(query);
		}
	}
}

void Cache::fetch_lastmodified(const std::string& feedurl,
	time_t& t,
	std::string& etag)
{
	std::lock_guard<std::mutex> lock(mtx);
	std::string query = prepare_query(
		"SELECT lastmodified, etag FROM RssFeed WHERE rssurl = '%q';",
		feedurl);
	header_values result = {0, ""};
	run_sql(query, lastmodified_callback, &result);
	t = result.lastmodified;
	etag = result.etag;
	LOG(Level::DEBUG,
		"Cache::fetch_lastmodified: t = %d etag = %s",
		t,
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
	std::lock_guard<std::mutex> lock(mtx);
	std::string query = "UPDATE RssFeed SET ";
	if (t > 0)
		query.append(prepare_query("lastmodified = '%d'", t));
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
	std::lock_guard<std::mutex> lock(mtx);
	std::string query = prepare_query(
		"UPDATE RssItem SET deleted = %u WHERE guid = '%q'",
		b ? 1 : 0,
		guid);
	run_sql_nothrow(query);
}

void Cache::mark_feed_items_deleted(const std::string& feedurl)
{
	std::lock_guard<std::mutex> lock(mtx);
	std::string query = prepare_query(
		"UPDATE RssItem SET deleted = 1 WHERE feedurl = '%s';",
		feedurl);
	run_sql_nothrow(query);
}

// this function writes an RssFeed including all RssItems to the database
void Cache::externalize_rssfeed(std::shared_ptr<RssFeed> feed,
	bool reset_unread)
{
	ScopeMeasure m1("Cache::externalize_feed");
	if (feed->is_query_feed()) {
		return;
	}

	std::lock_guard<std::mutex> lock(mtx);
	std::lock_guard<std::mutex> feedlock(feed->item_mutex);
	// scope_transaction dbtrans(db);

	cb_handler count_cbh;
	auto query = prepare_query(
		"SELECT count(*) FROM RssFeed WHERE rssurl = '%q';",
		feed->rssurl());
	run_sql(query, count_callback, &count_cbh);

	int count = count_cbh.count();
	LOG(Level::DEBUG,
		"Cache::externalize_RssFeed: RssFeeds with rssurl = '%s': "
		"found "
		"%d",
		feed->rssurl(),
		count);
	if (count > 0) {
		std::string updatequery = prepare_query(
			"UPDATE RssFeed "
			"SET title = '%q', url = '%q', is_rtl = %u "
			"WHERE rssurl = '%q';",
			feed->title_raw(),
			feed->link(),
			feed->is_rtl() ? 1 : 0,
			feed->rssurl());
		run_sql(updatequery);
	} else {
		std::string insertquery = prepare_query(
			"INSERT INTO RssFeed (rssurl, url, title, is_rtl) "
			"VALUES ( '%q', '%q', '%q', %u );",
			feed->rssurl(),
			feed->link(),
			feed->title_raw(),
			feed->is_rtl() ? 1 : 0);
		run_sql(insertquery);
	}

	unsigned int max_items = cfg->get_configvalue_as_int("max-items");

	LOG(Level::INFO,
		"Cache::externalize_feed: max_items = %u "
		"feed.total_item_count() = "
		"%u",
		max_items,
		feed->total_item_count());

	if (max_items > 0 && feed->total_item_count() > max_items) {
		feed->erase_items(
			feed->items().begin() + max_items, feed->items().end());
	}

	unsigned int days = cfg->get_configvalue_as_int("keep-articles-days");
	time_t old_time = time(nullptr) - days * 24 * 60 * 60;

	// the reverse iterator is there for the sorting foo below (think about
	// it)
	for (auto it = feed->items().rbegin(); it != feed->items().rend();
		++it) {
		if (days == 0 || (*it)->pubDate_timestamp() >= old_time)
			update_rssitem_unlocked(
				*it, feed->rssurl(), reset_unread);
	}
}

// this function reads an RssFeed including all of its RssItems.
// the feed parameter needs to have the rssurl member set.
std::shared_ptr<RssFeed> Cache::internalize_rssfeed(std::string rssurl,
	RssIgnores* ign)
{
	ScopeMeasure m1("Cache::internalize_rssfeed");

	std::shared_ptr<RssFeed> feed(new RssFeed(this));
	feed->set_rssurl(rssurl);

	if (Utils::is_query_url(rssurl)) {
		return feed;
	}

	std::lock_guard<std::mutex> lock(mtx);
	std::lock_guard<std::mutex> feedlock(feed->item_mutex);

	/* first, we check whether the feed is there at all */
	std::string query = prepare_query(
		"SELECT count(*) FROM RssFeed WHERE rssurl = '%q';", rssurl);
	cb_handler count_cbh;
	run_sql(query, count_callback, &count_cbh);

	if (count_cbh.count() == 0) {
		return feed;
	}

	/* then we first read the feed from the database */
	query = prepare_query(
		"SELECT title, url, is_rtl FROM RssFeed WHERE rssurl = '%q';",
		rssurl);
	run_sql(query, rssfeed_callback, &feed);

	/* ...and then the associated items */
	query = prepare_query(
		"SELECT guid, title, author, url, pubDate, length(content), "
		"unread, "
		"feedurl, enclosure_url, enclosure_type, enqueued, flags, base "
		"FROM RssItem "
		"WHERE feedurl = '%q' "
		"AND deleted = 0 "
		"ORDER BY pubDate DESC, id DESC;",
		rssurl);
	run_sql(query, rssitem_callback, &feed);

	std::vector<std::shared_ptr<RssItem>> filtered_items;
	for (const auto& item : feed->items()) {
		try {
			if (!ign || !ign->matches(item.get())) {
				item->set_cache(this);
				item->set_feedptr(feed);
				item->set_feedurl(feed->rssurl());
				filtered_items.push_back(item);
			}
		} catch (const MatcherException& ex) {
			LOG(Level::DEBUG,
				"oops, Matcher Exception: %s",
				ex.what());
		}
	}
	feed->set_items(filtered_items);

	unsigned int max_items = cfg->get_configvalue_as_int("max-items");

	if (max_items > 0 && feed->total_item_count() > max_items) {
		std::vector<std::shared_ptr<RssItem>> flagged_items;
		for (unsigned int j = max_items; j < feed->total_item_count();
			++j) {
			if (feed->items()[j]->flags().length() == 0) {
				delete_item(feed->items()[j]);
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

std::vector<std::shared_ptr<RssItem>>
Cache::search_for_items(const std::string& querystr, const std::string& feedurl)
{
	assert(!Utils::is_query_url(feedurl));
	std::string query;
	std::vector<std::shared_ptr<RssItem>> items;

	std::lock_guard<std::mutex> lock(mtx);
	if (feedurl.length() > 0) {
		query = prepare_query(
			"SELECT guid, title, author, url, pubDate, "
			"length(content), "
			"unread, feedurl, enclosure_url, enclosure_type, "
			"enqueued, flags, base "
			"FROM RssItem "
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
			"enqueued, flags, base "
			"FROM RssItem "
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

	return items;
}

std::unordered_set<std::string> Cache::search_in_items(
	const std::string& querystr,
	const std::unordered_set<std::string>& guids)
{
	std::string list = "(";

	for (const auto& guid : guids) {
		list.append(prepare_query("%Q, ", guid));
	}
	list.append("'')");

	std::string query = prepare_query(
		"SELECT guid "
		"FROM RssItem "
		"WHERE (title LIKE '%%%q%%' OR content LIKE '%%%q%%') "
		"AND guid IN %s;",
		querystr,
		querystr,
		list);

	std::unordered_set<std::string> items;
	std::lock_guard<std::mutex> lock(mtx);
	run_sql(query, guid_callback, &items);
	return items;
}

void Cache::delete_item(const std::shared_ptr<RssItem>& item)
{
	std::string query = prepare_query(
		"DELETE FROM RssItem WHERE guid = '%q';", item->guid());
	run_sql(query);
}

void Cache::do_vacuum()
{
	std::lock_guard<std::mutex> lock(mtx);
	run_sql("VACUUM;");
}

void Cache::cleanup_cache(std::vector<std::shared_ptr<RssFeed>>& feeds)
{
	mtx.lock(); // we don't use the std::lock_guard<> here... see comments
		    // below

	/*
	 * Cache cleanup means that all entries in both the RssFeed and
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
	if (cfg->get_configvalue_as_bool("cleanup-on-quit")) {
		LOG(Level::DEBUG, "Cache::cleanup_cache: cleaning up Cache...");
		std::string list = "(";

		for (const auto& feed : feeds) {
			std::string name =
				prepare_query("'%q'", feed->rssurl());
			list.append(name);
			list.append(", ");
		}
		list.append("'')");

		std::string cleanup_RssFeeds_statement(
			"DELETE FROM RssFeed WHERE rssurl NOT IN ");
		cleanup_RssFeeds_statement.append(list);
		cleanup_RssFeeds_statement.append(1, ';');

		std::string cleanup_RssItems_statement(
			"DELETE FROM RssItem WHERE feedurl NOT IN ");
		cleanup_RssItems_statement.append(list);
		cleanup_RssItems_statement.append(1, ';');

		std::string cleanup_read_items_statement(
			"UPDATE RssItem SET deleted = 1 WHERE unread = 0");

		run_sql(cleanup_RssFeeds_statement);
		run_sql(cleanup_RssItems_statement);
		if (cfg->get_configvalue_as_bool(
			    "delete-read-articles-on-quit")) {
			run_sql(cleanup_read_items_statement);
		}

		// WARNING: THE MISSING UNLOCK OPERATION IS MISSING FOR A
		// PURPOSE! It's missing so that no database operation can occur
		// after the Cache cleanup! mtx->unlock();
	} else {
		LOG(Level::DEBUG,
			"Cache::cleanup_cache: NOT cleaning up Cache...");
	}
}

void Cache::update_rssitem_unlocked(std::shared_ptr<RssItem> item,
	const std::string& feedurl,
	bool reset_unread)
{
	std::string query = prepare_query(
		"SELECT count(*) FROM RssItem WHERE guid = '%q';",
		item->guid());
	cb_handler count_cbh;
	run_sql(query, count_callback, &count_cbh);
	if (count_cbh.count() > 0) {
		if (reset_unread) {
			std::string content;
			query = prepare_query(
				"SELECT content FROM RssItem WHERE guid = "
				"'%q';",
				item->guid());
			run_sql(query, single_string_callback, &content);
			if (content != item->description_raw()) {
				LOG(Level::DEBUG,
					"Cache::update_rssitem_unlocked: '%s' "
					"is "
					"different from '%s'",
					content,
					item->description_raw());
				query = prepare_query(
					"UPDATE RssItem SET unread = 1 WHERE "
					"guid = '%q';",
					item->guid());
				run_sql(query);
			}
		}
		std::string update;
		if (item->override_unread()) {
			update = prepare_query(
				"UPDATE RssItem "
				"SET title = '%q', author = '%q', url = '%q', "
				"feedurl = '%q', "
				"content = '%q', enclosure_url = '%q', "
				"enclosure_type = '%q', base = '%q', unread = "
				"'%d' "
				"WHERE guid = '%q'",
				item->title_raw(),
				item->author_raw(),
				item->link(),
				feedurl,
				item->description_raw(),
				item->enclosure_url(),
				item->enclosure_type(),
				item->get_base(),
				(item->unread() ? 1 : 0),
				item->guid());
		} else {
			update = prepare_query(
				"UPDATE RssItem "
				"SET title = '%q', author = '%q', url = '%q', "
				"feedurl = '%q', "
				"content = '%q', enclosure_url = '%q', "
				"enclosure_type = '%q', base = '%q' "
				"WHERE guid = '%q'",
				item->title_raw(),
				item->author_raw(),
				item->link(),
				feedurl,
				item->description_raw(),
				item->enclosure_url(),
				item->enclosure_type(),
				item->get_base(),
				item->guid());
		}
		run_sql(update);
	} else {
		std::string insert = prepare_query(
			"INSERT INTO RssItem (guid, title, author, url, "
			"feedurl, "
			"pubDate, content, unread, enclosure_url, "
			"enclosure_type, enqueued, base) "
			"VALUES "
			"('%q','%q','%q','%q','%q','%u','%q','%d','%q','%q',%d,"
			" "
			"'%q')",
			item->guid(),
			item->title_raw(),
			item->author_raw(),
			item->link(),
			feedurl,
			item->pubDate_timestamp(),
			item->description_raw(),
			(item->unread() ? 1 : 0),
			item->enclosure_url(),
			item->enclosure_type(),
			item->enqueued() ? 1 : 0,
			item->get_base());
		run_sql(insert);
	}
}

void Cache::mark_all_read(std::shared_ptr<RssFeed> feed)
{
	std::lock_guard<std::mutex> lock(mtx);
	std::lock_guard<std::mutex> itemlock(feed->item_mutex);
	std::string query =
		"UPDATE RssItem SET unread = '0' WHERE unread != '0' AND guid "
		"IN (";

	for (const auto& item : feed->items()) {
		query.append(prepare_query("'%q',", item->guid()));
	}
	query.append("'');");

	run_sql(query);
}

/* this function marks all RssItems (optionally of a certain feed url) as read
 */
void Cache::mark_all_read(const std::string& feedurl)
{
	std::lock_guard<std::mutex> lock(mtx);

	std::string query;
	if (feedurl.length() > 0) {
		query = prepare_query(
			"UPDATE RssItem "
			"SET unread = '0' "
			"WHERE unread != '0' "
			"AND feedurl = '%q';",
			feedurl);
	} else {
		query = prepare_query(
			"UPDATE RssItem "
			"SET unread = '0' "
			"WHERE unread != '0';");
	}
	run_sql(query);
}

void Cache::update_rssitem_unread_and_enqueued(RssItem* item,
	const std::string& /* feedurl */)
{
	std::lock_guard<std::mutex> lock(mtx);

	auto query = prepare_query(
		"UPDATE RssItem "
		"SET unread = '%d', enqueued = '%d' "
		"WHERE guid = '%q'",
		item->unread() ? 1 : 0,
		item->enqueued() ? 1 : 0,
		item->guid());
	run_sql(query);
}

/* this function updates the unread and enqueued flags */
void Cache::update_rssitem_unread_and_enqueued(std::shared_ptr<RssItem> item,
	const std::string& feedurl)
{
	update_rssitem_unread_and_enqueued(item.get(), feedurl);
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
std::string
Cache::prepare_query(const std::string& format, const T& argument, Args... args)
{
	std::string local_format, remaining_format;
	std::tie(local_format, remaining_format) =
		StrPrintf::split_format(format);

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
	std::lock_guard<std::mutex> lock(mtx);

	std::string update = prepare_query(
		"UPDATE RssItem SET flags = '%q' WHERE guid = '%q';",
		item->flags(),
		item->guid());

	run_sql(update);
}

void Cache::remove_old_deleted_items(const std::string& rssurl,
	const std::vector<std::string>& guids)
{
	ScopeMeasure m1("Cache::remove_old_deleted_items");
	if (guids.size() == 0) {
		LOG(Level::DEBUG,
			"Cache::remove_old_deleted_items: not cleaning up "
			"anything because last reload brought no new items "
			"(detected "
			"no changes)");
		return;
	}
	std::string guidset = "(";
	for (const auto& guid : guids) {
		guidset.append(prepare_query("'%q', ", guid));
	}
	guidset.append("'')");
	std::string query = prepare_query(
		"DELETE FROM RssItem "
		"WHERE feedurl = '%q' "
		"AND deleted = 1 "
		"AND guid NOT IN %s;",
		rssurl,
		guidset);
	std::lock_guard<std::mutex> lock(mtx);
	run_sql(query);
}

unsigned int Cache::get_unread_count()
{
	std::lock_guard<std::mutex> lock(mtx);

	std::string countquery =
		"SELECT count(id) FROM RssItem WHERE unread = 1;";
	cb_handler count_cbh;
	run_sql(countquery, count_callback, &count_cbh);
	unsigned int count = static_cast<unsigned int>(count_cbh.count());
	LOG(Level::DEBUG, "Cache::get_unread_count: count = %u", count);
	return count;
}

void Cache::mark_items_read_by_guid(const std::vector<std::string>& guids)
{
	ScopeMeasure m1("Cache::mark_items_read_by_guid");
	std::string guidset("(");
	for (const auto& guid : guids) {
		guidset.append(prepare_query("'%q', ", guid));
	}
	guidset.append("'')");

	std::string updatequery = prepare_query(
		"UPDATE RssItem SET unread = 0 WHERE unread = 1 AND guid IN "
		"%s;",
		guidset);

	std::lock_guard<std::mutex> lock(mtx);
	run_sql(updatequery);
}

std::vector<std::string> Cache::get_read_item_guids()
{
	std::vector<std::string> guids;
	std::string query = "SELECT guid FROM RssItem WHERE unread = 0;";

	std::lock_guard<std::mutex> lock(mtx);
	run_sql(query, vectorofstring_callback, &guids);

	return guids;
}

void Cache::clean_old_articles()
{
	std::lock_guard<std::mutex> lock(mtx);

	unsigned int days = cfg->get_configvalue_as_int("keep-articles-days");
	if (days > 0) {
		time_t old_date = time(nullptr) - days * 24 * 60 * 60;

		std::string query(prepare_query(
			"DELETE FROM RssItem WHERE pubDate < %d", old_date));
		LOG(Level::DEBUG,
			"Cache::clean_old_articles: about to delete articles "
			"with a pubDate older than %d",
			old_date);
		run_sql(query);
	} else {
		LOG(Level::DEBUG,
			"Cache::clean_old_articles, days == 0, not cleaning up "
			"anything");
	}
}

void Cache::fetch_descriptions(RssFeed* feed)
{
	std::vector<std::string> guids;
	for (const auto& item : feed->items()) {
		guids.push_back(prepare_query("'%q'", item->guid()));
	}
	std::string in_clause = Utils::join(guids, ", ");

	std::string query = prepare_query(
		"SELECT guid, content FROM RssItem WHERE guid IN (%s);",
		in_clause);

	run_sql(query, fill_content_callback, feed);
}

schema_version Cache::get_schema_version()
{
	sqlite3_stmt* stmt{};
	schema_version result;

	int rc = sqlite3_prepare_v2(db,
		"SELECT db_schema_version_major, db_schema_version_minor "
		"FROM metadata",
		-1,
		&stmt,
		nullptr);

	if (rc != SQLITE_OK) {
		// I'm pretty sure the query above is correct, and the only way
		// it can fail is when metadata table is not present in the DB.
		// That means we're dealing with an empty Cache file, or one
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

} // namespace newsboat

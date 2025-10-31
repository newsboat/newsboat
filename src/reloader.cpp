#include "reloader.h"

#include <algorithm>
#include <iostream>
#include <ncurses.h>
#include <thread>

#include "controller.h"
#include "curlhandle.h"
#include "dbexception.h"
#include "feedretriever.h"
#include "fmtstrformatter.h"
#include "logger.h"
#include "matcherexception.h"
#include "reloadthread.h"
#include "rss/exception.h"
#include "rssfeed.h"
#include "rssparser.h"
#include "scopemeasure.h"
#include "utils.h"
#include "view.h"

namespace newsboat {

Reloader::Reloader(Controller& c, Cache& cc, ConfigContainer& cfg)
	: ctrl(c)
	, rsscache(cc)
	, cfg(cfg)
{
}

void Reloader::spawn_reloadthread()
{
	std::thread t{ReloadThread(ctrl, cfg)};
	t.detach();
}

void Reloader::start_reload_all_thread(const std::vector<unsigned int>& indexes)
{
	LOG(Level::INFO, "starting reload all thread");
	std::thread t([=]() {
		LOG(Level::DEBUG,
			"Reloader::start_reload_all_thread: inside thread, reloading all "
			"feeds...");
		if (trylock_reload_mutex()) {
			RssFeedRegistry::get_instance()->start_new_generation();
			RssItemRegistry::get_instance()->start_new_generation();

			if (indexes.empty()) {
				reload_all();
			} else {
				reload_indexes(indexes);
			}
			unlock_reload_mutex();

			RssFeedRegistry::get_instance()->print_report();
			RssItemRegistry::get_instance()->print_report();
		}
	});
	t.detach();
}

bool Reloader::trylock_reload_mutex()
{
	if (reload_mutex.try_lock()) {
		LOG(Level::DEBUG, "Reloader::trylock_reload_mutex succeeded");
		return true;
	}
	LOG(Level::DEBUG, "Reloader::trylock_reload_mutex failed");
	return false;
}

void Reloader::reload(unsigned int pos,
	CurlHandle& easyhandle,
	bool show_progress,
	bool unattended)
{
	ScopeMeasure sm("Reloader::reload");
	LOG(Level::DEBUG, "Reloader::reload: pos = %u", pos);
	std::shared_ptr<RssFeed> oldfeed = ctrl.get_feedcontainer()->get_feed(pos);
	if (oldfeed) {
		LOG(Level::INFO, "Reloader::reload: starting reload of %s", oldfeed->rssurl());

		// Query feed reloading should be handled by the calling functions
		// (e.g.  Reloader::reload_all() calling View::prepare_query_feed())
		if (oldfeed->is_query_feed()) {
			LOG(Level::DEBUG, "Reloader::reload: skipping query feed");
			return;
		}

		std::string errmsg;
		std::shared_ptr<AutoDiscardMessage> message_lifetime;
		if (!unattended) {
			const std::string progress = show_progress ?
				strprintf::fmt("(%u/%u) ", ++reload_progress, reload_progress_max) :
				"";
			message_lifetime = ctrl.get_view()->get_statusline().show_message_until_finished(
					strprintf::fmt(_("%sLoading %s..."),
						progress,
						utils::censor_url(oldfeed->rssurl())));
		}

		const bool ignore_dl =
			(cfg.get_configvalue("ignore-mode") == "download");

		try {
			const auto inner_message_lifetime = message_lifetime;
			message_lifetime.reset();
			oldfeed->set_status(DlStatus::DURING_DOWNLOAD);

			RssIgnores* ign = ignore_dl ? ctrl.get_ignores() : nullptr;

			LOG(Level::INFO, "Reloader::reload: retrieving feed");
			sm.stopover("start retrieving");
			FeedRetriever feed_retriever(cfg, rsscache, easyhandle, ign, ctrl.get_api());
			const rsspp::Feed feed = feed_retriever.retrieve(oldfeed->rssurl());

			LOG(Level::INFO, "Reloader::reload: parsing feed");
			sm.stopover("start parsing");
			RssParser parser(oldfeed->rssurl(), rsscache, cfg, ign);

			std::shared_ptr<RssFeed> newfeed = parser.parse(feed);
			sm.stopover("start replacing feed");
			if (newfeed != nullptr) {
				ctrl.replace_feed(
					*oldfeed, *newfeed, pos, unattended);
				if (newfeed->total_item_count() == 0) {
					LOG(Level::DEBUG,
						"Reloader::reload: feed is empty");
				}
			}
			oldfeed->set_status(DlStatus::SUCCESS);
		} catch (const DbException& e) {
			errmsg = strprintf::fmt(
					_("Error while retrieving %s: %s"),
					utils::censor_url(oldfeed->rssurl()),
					e.what());
		} catch (const std::string& emsg) {
			errmsg = strprintf::fmt(
					_("Error while retrieving %s: %s"),
					utils::censor_url(oldfeed->rssurl()),
					emsg);
		} catch (const rsspp::Exception& e) {
			errmsg = strprintf::fmt(
					_("Error while retrieving %s: %s"),
					utils::censor_url(oldfeed->rssurl()),
					e.what());
		} catch (const rsspp::NotModifiedException&) {
			// Nothing to be done, feed was not chaned since last retrieve
			oldfeed->set_status(DlStatus::SUCCESS);
		}
		if (!errmsg.empty()) {
			oldfeed->set_status(DlStatus::DL_ERROR);
			ctrl.get_view()->get_statusline().show_error(errmsg);
			LOG(Level::USERERROR, "%s", errmsg);
		}
	} else {
		ctrl.get_view()->get_statusline().show_error(_("Error: invalid feed!"));
	}
}

void Reloader::partition_reload_to_threads(
	std::function<void(unsigned int start, unsigned int end)> handle_range,
	unsigned int num_feeds)
{
	int num_threads = cfg.get_configvalue_as_int("reload-threads");
	// TODO: change to std::clamp in C++17
	const int min_threads = 1;
	const int max_threads = num_feeds;
	num_threads = std::max(min_threads, std::min(num_threads, max_threads));

	LOG(Level::DEBUG, "Reloader::partition_reload_to_threads: starting with reload...");
	reload_progress = 0;
	reload_progress_max = num_feeds;
	if (num_threads == 1) {
		handle_range(0, num_feeds - 1);
	} else {
		std::vector<std::pair<unsigned int, unsigned int>> partitions =
				utils::partition_indexes(0, num_feeds - 1, num_threads);
		std::vector<std::thread> threads;
		LOG(Level::DEBUG,
			"Reloader::partition_reload_to_threads: starting reload threads...");
		for (int i = 0; i < num_threads - 1; i++) {
			auto range = partitions[i];
			threads.emplace_back([=]() {
				handle_range(range.first, range.second);
			});
		}
		LOG(Level::DEBUG,
			"Reloader::partition_reload_to_threads: starting my own reload...");
		handle_range(partitions[num_threads - 1].first,
			partitions[num_threads - 1].second);
		LOG(Level::DEBUG,
			"Reloader::partition_reload_to_threads: joining other threads...");
		for (size_t i = 0; i < threads.size(); i++) {
			threads[i].join();
		}
	}
}

void Reloader::reload_all(bool unattended)
{
	ScopeMeasure sm("Reloader::reload_all");

	const auto unread_feeds =
		ctrl.get_feedcontainer()->unread_feed_count();
	const auto unread_articles =
		ctrl.get_feedcontainer()->unread_item_count();

	ctrl.get_feedcontainer()->reset_feeds_status();
	const auto num_feeds = ctrl.get_feedcontainer()->feeds_size();

	std::vector<unsigned int> v;
	for (unsigned int i = 0; i < num_feeds; ++i) {
		v.push_back(i);
	}
	reload_indexes_impl(v, unattended);

	// refresh query feeds (update and sort)
	LOG(Level::DEBUG, "Reloader::reload_all: refresh query feeds");
	for (const auto& feed : ctrl.get_feedcontainer()->get_all_feeds()) {
		if (feed->is_query_feed()) {
			try {
				ctrl.get_view()->prepare_query_feed(feed);
				feed->set_status(DlStatus::SUCCESS);
			} catch (const MatcherException& /* e */) {
				feed->set_status(DlStatus::DL_ERROR);
			}
		}
	}

	ctrl.get_feedcontainer()->sort_feeds(cfg.get_feed_sort_strategy());
	ctrl.update_feedlist();
	ctrl.get_view()->force_redraw();

	notify_reload_finished(unread_feeds, unread_articles);
}

void Reloader::reload_indexes_impl(std::vector<unsigned int> indexes, bool unattended)
{
	auto extract = [](std::string& s, const std::string& url) {
		size_t p = url.find("//");
		p = (p == std::string::npos) ? 0 : p + 2;
		std::string suff(url.substr(p));
		p = suff.find('/');
		s = suff.substr(0, p);
	};

	const auto feeds = ctrl.get_feedcontainer()->get_all_feeds();

	// Sort the feeds based on domain, so feeds on the same domain
	// can share the curl handle.
	//
	// See commit: 115cf667485929bbb698ba8051dec7ff6a73739c
	std::sort(indexes.begin(), indexes.end(), [&](unsigned int a, unsigned int b) {
		std::string domain1, domain2;
		extract(domain1, feeds[a]->rssurl());
		extract(domain2, feeds[b]->rssurl());
		std::reverse(domain1.begin(), domain1.end());
		std::reverse(domain2.begin(), domain2.end());
		return domain1 < domain2;
	});

	partition_reload_to_threads([&](unsigned int start, unsigned int end) {
		CurlHandle easyhandle;
		for (auto i = start; i <= end; ++i) {
			unsigned int feed_index = indexes[i];
			LOG(Level::DEBUG,
				"Reloader::reload_indexes_impl: reloading feed #%u",
				feed_index);
			reload(feed_index, easyhandle, true, unattended);

			// Reset any options set on the handle before next reload
			curl_easy_reset(easyhandle.ptr());

			// Restore cookiejar config to make sure option is active during curl_easy_cleanup()
			const auto cookie_cache = cfg.get_configvalue("cookie-cache");
			if (cookie_cache != "") {
				curl_easy_setopt(easyhandle.ptr(), CURLOPT_COOKIEJAR, cookie_cache.c_str());
			}
		}
	}, indexes.size());
}

void Reloader::reload_indexes(const std::vector<unsigned int>& indexes, bool unattended)
{
	ScopeMeasure m1("Reloader::reload_indexes");
	const auto unread_feeds =
		ctrl.get_feedcontainer()->unread_feed_count();
	const auto unread_articles =
		ctrl.get_feedcontainer()->unread_item_count();

	reload_indexes_impl(indexes, unattended);

	notify_reload_finished(unread_feeds, unread_articles);
}

void Reloader::notify(const std::string& msg)
{
	if (cfg.get_configvalue_as_bool("notify-screen")) {
		LOG(Level::DEBUG, "reloader:notify: notifying screen");
		std::cout << "\033^" << msg << "\033\\";
		std::cout.flush();
	}
	if (cfg.get_configvalue_as_bool("notify-xterm")) {
		LOG(Level::DEBUG, "reloader:notify: notifying xterm");
		std::cout << "\033]2;" << msg << "\033\\";
		std::cout.flush();
	}
	if (cfg.get_configvalue_as_bool("notify-beep")) {
		LOG(Level::DEBUG, "reloader:notify: notifying beep");
		::beep();
	}
	const auto notify_program = cfg.get_configvalue_as_filepath("notify-program");
	if (notify_program != Filepath{}) {
		LOG(Level::DEBUG,
			"reloader:notify: notifying external program `%s'",
			notify_program);
		utils::run_command(notify_program.to_locale_string(), msg);
	}
}

void Reloader::notify_reload_finished(unsigned int unread_feeds_before,
	unsigned int unread_articles_before)
{
	const auto unread_feeds =
		ctrl.get_feedcontainer()->unread_feed_count();
	const auto unread_articles =
		ctrl.get_feedcontainer()->unread_item_count();
	const bool notify_always = cfg.get_configvalue_as_bool("notify-always");

	if (notify_always || unread_feeds > unread_feeds_before ||
		unread_articles > unread_articles_before) {
		// TODO: Determine what should be done if `unread_articles < unread_articles_before`.
		// It is expected this can happen if the user marks an article as read
		// while a reload is in progress.
		const int article_count = unread_articles - unread_articles_before;
		const int feed_count = unread_feeds - unread_feeds_before;

		LOG(Level::DEBUG, "unread article count: %d", article_count);
		LOG(Level::DEBUG, "unread feed count: %d", feed_count);

		FmtStrFormatter fmt;
		fmt.register_fmt('f', std::to_string(unread_feeds));
		fmt.register_fmt('n', std::to_string(unread_articles));
		fmt.register_fmt('d',
			std::to_string(article_count >= 0 ? article_count : 0));
		fmt.register_fmt(
			'D', std::to_string(feed_count >= 0 ? feed_count : 0));
		notify(fmt.do_format(cfg.get_configvalue("notify-format")));
	}
}

} // namespace newsboat

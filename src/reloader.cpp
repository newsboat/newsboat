#include "reloader.h"

#include <algorithm>
#include <cinttypes>
#include <iostream>
#include <ncurses.h>
#include <thread>

#include "controller.h"
#include "curlhandle.h"
#include "dbexception.h"
#include "downloadthread.h"
#include "fmtstrformatter.h"
#include "matcherexception.h"
#include "reloadrangethread.h"
#include "reloadthread.h"
#include "rss/exception.h"
#include "rssfeed.h"
#include "rssparser.h"
#include "scopemeasure.h"
#include "utils.h"
#include "view.h"

namespace newsboat {

Reloader::Reloader(Controller* c, Cache* cc, ConfigContainer* cfg)
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

void Reloader::start_reload_all_thread(const std::vector<int>& indexes)
{
	LOG(Level::INFO, "starting reload all thread");
	std::thread t(DownloadThread(*this, indexes));
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
	bool show_progress,
	bool unattended,
	CurlHandle* easyhandle)
{
	LOG(Level::DEBUG, "Reloader::reload: pos = %u", pos);
	std::shared_ptr<RssFeed> oldfeed = ctrl->get_feedcontainer()->get_feed(pos);
	if (oldfeed) {
		// Query feed reloading should be handled by the calling functions
		// (e.g.  Reloader::reload_all() calling View::prepare_query_feed())
		if (oldfeed->is_query_feed()) {
			LOG(Level::DEBUG, "Reloader::reload: skipping query feed");
			return;
		}

		std::string errmsg;
		std::shared_ptr<AutoDiscardMessage> message_lifetime;
		if (!unattended) {
			const auto max = ctrl->get_feedcontainer()->feeds_size();
			const std::string progress = show_progress ?
				strprintf::fmt("(%u/%u) ", pos + 1, max) :
				"";
			message_lifetime = ctrl->get_view()->get_statusline().show_message_until_finished(
					strprintf::fmt(_("%sLoading %s..."),
						progress,
						utils::censor_url(oldfeed->rssurl())));
		}

		const bool ignore_dl =
			(cfg->get_configvalue("ignore-mode") == "download");

		RssParser parser(oldfeed->rssurl(),
			rsscache,
			cfg,
			ignore_dl ? ctrl->get_ignores() : nullptr,
			ctrl->get_api());
		parser.set_easyhandle(easyhandle);
		LOG(Level::DEBUG, "Reloader::reload: created parser");
		try {
			const auto inner_message_lifetime = message_lifetime;
			message_lifetime.reset();
			oldfeed->set_status(DlStatus::DURING_DOWNLOAD);
			std::shared_ptr<RssFeed> newfeed = parser.parse();
			if (newfeed != nullptr) {
				ctrl->replace_feed(
					oldfeed, newfeed, pos, unattended);
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
			ctrl->get_view()->get_statusline().show_error(errmsg);
			LOG(Level::USERERROR, "%s", errmsg);
		}
	} else {
		ctrl->get_view()->get_statusline().show_error(_("Error: invalid feed!"));
	}
}

void Reloader::reload_all(bool unattended)
{
	ScopeMeasure sm("Reloader::reload_all");

	const auto unread_feeds =
		ctrl->get_feedcontainer()->unread_feed_count();
	const auto unread_articles =
		ctrl->get_feedcontainer()->unread_item_count();
	int num_threads = cfg->get_configvalue_as_int("reload-threads");

	ctrl->get_feedcontainer()->reset_feeds_status();
	const auto num_feeds = ctrl->get_feedcontainer()->feeds_size();

	// TODO: change to std::clamp in C++17
	const int min_threads = 1;
	const int max_threads = num_feeds;
	num_threads = std::max(min_threads, std::min(num_threads, max_threads));

	LOG(Level::DEBUG, "Reloader::reload_all: starting with reload all...");
	if (num_threads == 1) {
		reload_range(0, num_feeds - 1, unattended);
	} else {
		std::vector<std::pair<unsigned int, unsigned int>> partitions =
				utils::partition_indexes(0, num_feeds - 1, num_threads);
		std::vector<std::thread> threads;
		LOG(Level::DEBUG,
			"Reloader::reload_all: starting reload threads...");
		for (int i = 0; i < num_threads - 1; i++) {
			threads.push_back(std::thread(ReloadRangeThread(*this,
						partitions[i].first,
						partitions[i].second,
						unattended)));
		}
		LOG(Level::DEBUG,
			"Reloader::reload_all: starting my own reload...");
		reload_range(partitions[num_threads - 1].first,
			partitions[num_threads - 1].second,
			unattended);
		LOG(Level::DEBUG,
			"Reloader::reload_all: joining other threads...");
		for (size_t i = 0; i < threads.size(); i++) {
			threads[i].join();
		}
	}

	// refresh query feeds (update and sort)
	LOG(Level::DEBUG, "Reloader::reload_all: refresh query feeds");
	for (const auto& feed : ctrl->get_feedcontainer()->get_all_feeds()) {
		if (feed->is_query_feed()) {
			try {
				ctrl->get_view()->prepare_query_feed(feed);
				feed->set_status(DlStatus::SUCCESS);
			} catch (const MatcherException& /* e */) {
				feed->set_status(DlStatus::DL_ERROR);
			}
		}
	}

	ctrl->get_feedcontainer()->sort_feeds(cfg->get_feed_sort_strategy());
	ctrl->update_feedlist();
	ctrl->get_view()->force_redraw();

	notify_reload_finished(unread_feeds, unread_articles);
}

void Reloader::reload_indexes(const std::vector<int>& indexes, bool unattended)
{
	ScopeMeasure m1("Reloader::reload_indexes");
	const auto unread_feeds =
		ctrl->get_feedcontainer()->unread_feed_count();
	const auto unread_articles =
		ctrl->get_feedcontainer()->unread_item_count();

	for (const auto& idx : indexes) {
		reload(idx, true, unattended);
	}

	notify_reload_finished(unread_feeds, unread_articles);
}

void Reloader::reload_range(unsigned int start,
	unsigned int end,
	bool unattended)
{
	std::vector<unsigned int> v;
	for (unsigned int i = start; i <= end; ++i) {
		v.push_back(i);
	}

	auto extract = [](std::string& s, const std::string& url) {
		size_t p = url.find("//");
		p = (p == std::string::npos) ? 0 : p + 2;
		std::string suff(url.substr(p));
		p = suff.find('/');
		s = suff.substr(0, p);
	};

	const auto feeds = ctrl->get_feedcontainer()->get_all_feeds();

	std::sort(v.begin(), v.end(), [&](unsigned int a, unsigned int b) {
		std::string domain1, domain2;
		extract(domain1, feeds[a]->rssurl());
		extract(domain2, feeds[b]->rssurl());
		std::reverse(domain1.begin(), domain1.end());
		std::reverse(domain2.begin(), domain2.end());
		return domain1 < domain2;
	});

	CurlHandle easyhandle;

	for (const auto& i : v) {
		LOG(Level::DEBUG,
			"Reloader::reload_range: reloading feed #%u",
			i);
		reload(i, true, unattended, &easyhandle);
	}
}

void Reloader::notify(const std::string& msg)
{
	if (cfg->get_configvalue_as_bool("notify-screen")) {
		LOG(Level::DEBUG, "reloader:notify: notifying screen");
		std::cout << "\033^" << msg << "\033\\";
		std::cout.flush();
	}
	if (cfg->get_configvalue_as_bool("notify-xterm")) {
		LOG(Level::DEBUG, "reloader:notify: notifying xterm");
		std::cout << "\033]2;" << msg << "\033\\";
		std::cout.flush();
	}
	if (cfg->get_configvalue_as_bool("notify-beep")) {
		LOG(Level::DEBUG, "reloader:notify: notifying beep");
		::beep();
	}
	if (cfg->get_configvalue("notify-program").length() > 0) {
		std::string prog = cfg->get_configvalue("notify-program");
		LOG(Level::DEBUG,
			"reloader:notify: notifying external program `%s'",
			prog);
		utils::run_command(prog, msg);
	}
}

void Reloader::notify_reload_finished(unsigned int unread_feeds_before,
	unsigned int unread_articles_before)
{
	const auto unread_feeds =
		ctrl->get_feedcontainer()->unread_feed_count();
	const auto unread_articles =
		ctrl->get_feedcontainer()->unread_item_count();
	const bool notify_always = cfg->get_configvalue_as_bool("notify-always");

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
		notify(fmt.do_format(cfg->get_configvalue("notify-format")));
	}
}

} // namespace newsboat

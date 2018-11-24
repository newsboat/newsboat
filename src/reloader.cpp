#include "reloader.h"

#include <algorithm>
#include <iostream>
#include <ncurses.h>
#include <thread>

#include "controller.h"
#include "downloadthread.h"
#include "exceptions.h"
#include "fmtstrformatter.h"
#include "reloadrangethread.h"
#include "reloadthread.h"
#include "rss/rsspp.h"
#include "rssparser.h"
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

void Reloader::start_reload_all_thread(std::vector<int>* indexes)
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
	unsigned int max,
	bool unattended,
	CurlHandle* easyhandle)
{
	LOG(Level::DEBUG, "Reloader::reload: pos = %u max = %u", pos, max);
	if (pos < ctrl->get_feedcontainer()->feeds.size()) {
		std::shared_ptr<RssFeed> oldfeed =
			ctrl->get_feedcontainer()->feeds[pos];
		std::string errmsg;
		if (!unattended) {
			ctrl->get_view()->set_status(
				strprintf::fmt(_("%sLoading %s..."),
					prepare_message(pos + 1, max),
					utils::censor_url(oldfeed->rssurl())));
		}

		bool ignore_dl =
			(cfg->get_configvalue("ignore-mode") == "download");

		RssParser parser(oldfeed->rssurl(),
			rsscache,
			cfg,
			ignore_dl ? ctrl->get_ignores() : nullptr,
			ctrl->get_api());
		parser.set_easyhandle(easyhandle);
		LOG(Level::DEBUG, "Reloader::reload: created parser");
		try {
			oldfeed->set_status(DlStatus::DURING_DOWNLOAD);
			std::shared_ptr<RssFeed> newfeed = parser.parse();
			if (newfeed->total_item_count() > 0) {
				ctrl->replace_feed(
					oldfeed, newfeed, pos, unattended);
			} else {
				LOG(Level::DEBUG,
					"Reloader::reload: feed is empty");
			}
			oldfeed->set_status(DlStatus::SUCCESS);
			ctrl->get_view()->set_status("");
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
		} catch (rsspp::Exception& e) {
			errmsg = strprintf::fmt(
				_("Error while retrieving %s: %s"),
				utils::censor_url(oldfeed->rssurl()),
				e.what());
		}
		if (errmsg != "") {
			oldfeed->set_status(DlStatus::DL_ERROR);
			ctrl->get_view()->set_status(errmsg);
			LOG(Level::USERERROR, "%s", errmsg);
		}
	} else {
		ctrl->get_view()->show_error(_("Error: invalid feed!"));
	}
}

std::string Reloader::prepare_message(unsigned int pos, unsigned int max)
{
	if (max > 0) {
		return strprintf::fmt("(%u/%u) ", pos, max);
	}
	return "";
}

void Reloader::reload_all(bool unattended)
{
	const auto unread_feeds =
		ctrl->get_feedcontainer()->unread_feed_count();
	const auto unread_articles =
		ctrl->get_feedcontainer()->unread_item_count();
	int num_threads = cfg->get_configvalue_as_int("reload-threads");
	time_t t1, t2, dt;

	ctrl->get_feedcontainer()->reset_feeds_status();
	const auto num_feeds = ctrl->get_feedcontainer()->feeds_size();

	// TODO: change to std::clamp in C++17
	const int min_threads = 1;
	const int max_threads = num_feeds;
	num_threads = std::max(min_threads, std::min(num_threads, max_threads));

	t1 = time(nullptr);

	LOG(Level::DEBUG, "Reloader::reload_all: starting with reload all...");
	if (num_threads == 1) {
		reload_range(0, num_feeds - 1, num_feeds, unattended);
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
				num_feeds,
				unattended)));
		}
		LOG(Level::DEBUG,
			"Reloader::reload_all: starting my own reload...");
		reload_range(partitions[num_threads - 1].first,
			partitions[num_threads - 1].second,
			num_feeds,
			unattended);
		LOG(Level::DEBUG,
			"Reloader::reload_all: joining other threads...");
		for (size_t i = 0; i < threads.size(); i++) {
			threads[i].join();
		}
	}

	// refresh query feeds (update and sort)
	LOG(Level::DEBUG, "Reloader::reload_all: refresh query feeds");
	for (const auto& feed : ctrl->get_feedcontainer()->feeds) {
		ctrl->get_view()->prepare_query_feed(feed);
	}
	ctrl->get_view()->force_redraw();

	ctrl->get_feedcontainer()->sort_feeds(cfg->get_feed_sort_strategy());
	ctrl->update_feedlist();

	t2 = time(nullptr);
	dt = t2 - t1;
	LOG(Level::INFO, "Reloader::reload_all: reload took %d seconds", dt);

	const auto unread_feeds2 =
		ctrl->get_feedcontainer()->unread_feed_count();
	const auto unread_articles2 =
		ctrl->get_feedcontainer()->unread_item_count();
	bool notify_always = cfg->get_configvalue_as_bool("notify-always");
	if (notify_always || unread_feeds2 > unread_feeds ||
		unread_articles2 > unread_articles) {
		int article_count = unread_articles2 - unread_articles;
		int feed_count = unread_feeds2 - unread_feeds;

		LOG(Level::DEBUG, "unread article count: %d", article_count);
		LOG(Level::DEBUG, "unread feed count: %d", feed_count);

		FmtStrFormatter fmt;
		fmt.register_fmt('f', std::to_string(unread_feeds2));
		fmt.register_fmt('n', std::to_string(unread_articles2));
		fmt.register_fmt('d',
			std::to_string(article_count >= 0 ? article_count : 0));
		fmt.register_fmt(
			'D', std::to_string(feed_count >= 0 ? feed_count : 0));
		notify(fmt.do_format(cfg->get_configvalue("notify-format")));
	}
}

void Reloader::reload_indexes(const std::vector<int>& indexes, bool unattended)
{
	ScopeMeasure m1("Reloader::reload_indexes");
	const auto unread_feeds =
		ctrl->get_feedcontainer()->unread_feed_count();
	const auto unread_articles =
		ctrl->get_feedcontainer()->unread_item_count();
	const auto size = ctrl->get_feedcontainer()->feeds_size();

	for (const auto& idx : indexes) {
		reload(idx, size, unattended);
	}

	const auto unread_feeds2 =
		ctrl->get_feedcontainer()->unread_feed_count();
	const auto unread_articles2 =
		ctrl->get_feedcontainer()->unread_item_count();
	bool notify_always = cfg->get_configvalue_as_bool("notify-always");
	if (notify_always || unread_feeds2 != unread_feeds ||
		unread_articles2 != unread_articles) {
		FmtStrFormatter fmt;
		fmt.register_fmt('f', std::to_string(unread_feeds2));
		fmt.register_fmt('n', std::to_string(unread_articles2));
		fmt.register_fmt('d',
			std::to_string(unread_articles2 - unread_articles));
		fmt.register_fmt(
			'D', std::to_string(unread_feeds2 - unread_feeds));
		notify(fmt.do_format(cfg->get_configvalue("notify-format")));
	}
	if (!unattended) {
		ctrl->get_view()->set_status("");
	}
}

void Reloader::reload_range(unsigned int start,
	unsigned int end,
	unsigned int size,
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

	std::sort(v.begin(), v.end(), [&](unsigned int a, unsigned int b) {
		std::string domain1, domain2;
		extract(domain1, ctrl->get_feedcontainer()->feeds[a]->rssurl());
		extract(domain2, ctrl->get_feedcontainer()->feeds[b]->rssurl());
		std::reverse(domain1.begin(), domain1.end());
		std::reverse(domain2.begin(), domain2.end());
		return domain1 < domain2;
	});

	CurlHandle easyhandle;

	for (const auto& i : v) {
		LOG(Level::DEBUG,
			"Reloader::reload_range: reloading feed #%u",
			i);
		reload(i, size, unattended, &easyhandle);
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

} // namespace newsboat

#include "reloader.h"

#include <thread>

#include "controller.h"
#include "downloadthread.h"
#include "exceptions.h"
#include "reloadthread.h"
#include "rss/rsspp.h"
#include "rss_parser.h"
#include "utils.h"
#include "view.h"

namespace newsboat {

Reloader::Reloader(controller* c)
	: ctrl(c)
{
}

void Reloader::spawn_reloadthread()
{
	std::thread t{reloadthread(ctrl, ctrl->get_cfg())};
	t.detach();
}

void Reloader::start_reload_all_thread(std::vector<int>* indexes)
{
	LOG(level::INFO, "starting reload all thread");
	std::thread t(downloadthread(ctrl, indexes));
	t.detach();
}

bool Reloader::trylock_reload_mutex()
{
	if (reload_mutex.try_lock()) {
		LOG(level::DEBUG, "Reloader::trylock_reload_mutex succeeded");
		return true;
	}
	LOG(level::DEBUG, "Reloader::trylock_reload_mutex failed");
	return false;
}

void Reloader::reload(unsigned int pos,
	unsigned int max,
	bool unattended,
	curl_handle* easyhandle)
{
	LOG(level::DEBUG, "Reloader::reload: pos = %u max = %u", pos, max);
	if (pos < ctrl->get_feedcontainer()->feeds.size()) {
		std::shared_ptr<rss_feed> oldfeed =
			ctrl->get_feedcontainer()->feeds[pos];
		std::string errmsg;
		if (!unattended)
			ctrl->get_view()->set_status(
				strprintf::fmt(_("%sLoading %s..."),
					prepare_message(pos + 1, max),
					utils::censor_url(oldfeed->rssurl())));

		bool ignore_dl = (ctrl->get_cfg()->get_configvalue(
					  "ignore-mode") == "download");

		rss_parser parser(oldfeed->rssurl(),
			ctrl->get_cache(),
			ctrl->get_cfg(),
			ignore_dl ? ctrl->get_ignores() : nullptr,
			ctrl->get_api());
		parser.set_easyhandle(easyhandle);
		LOG(level::DEBUG, "Reloader::reload: created parser");
		try {
			oldfeed->set_status(dl_status::DURING_DOWNLOAD);
			std::shared_ptr<rss_feed> newfeed = parser.parse();
			if (newfeed->total_item_count() > 0) {
				ctrl->replace_feed(
					oldfeed, newfeed, pos, unattended);
			} else {
				LOG(level::DEBUG,
					"Reloader::reload: feed is empty");
			}
			oldfeed->set_status(dl_status::SUCCESS);
			ctrl->get_view()->set_status("");
		} catch (const dbexception& e) {
			errmsg = strprintf::fmt(
				_("Error while retrieving %s: %s"),
				utils::censor_url(oldfeed->rssurl()),
				e.what());
		} catch (const std::string& emsg) {
			errmsg = strprintf::fmt(
				_("Error while retrieving %s: %s"),
				utils::censor_url(oldfeed->rssurl()),
				emsg);
		} catch (rsspp::exception& e) {
			errmsg = strprintf::fmt(
				_("Error while retrieving %s: %s"),
				utils::censor_url(oldfeed->rssurl()),
				e.what());
		}
		if (errmsg != "") {
			oldfeed->set_status(dl_status::DL_ERROR);
			ctrl->get_view()->set_status(errmsg);
			LOG(level::USERERROR, "%s", errmsg);
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

} // namespace newsboat

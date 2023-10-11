#include "download.h"

#include <string>

#include "config.h"

namespace podboat {

/*
 * the Download class represents a single download entry in podboat.
 * It manages the filename, the URL, the current state, the progress, etc.
 */

Download::Download(std::function<void()> cb_require_view_update_)
	: download_status(DlStatus::QUEUED)
	, cursize(0.0)
	, totalsize(0.0)
	, curkbps(0.0)
	, offs(0)
	, cb_require_view_update(cb_require_view_update_)
{
}

Download::Download(const Download& download) :
	url_(download.url_),
	download_status(download.download_status),
	msg(download.msg),
	cursize(download.cursize),
	totalsize(download.totalsize),
	curkbps(download.curkbps),
	offs(download.offs),
	cb_require_view_update(download.cb_require_view_update)
{
	fn = download.fn.clone();
}

Download& Download::operator=(const Download& download)
{
	if (this == &download) {
		return *this;
	}
	url_ = download.url_;
	download_status = download.download_status;
	msg = download.msg;
	cursize = download.cursize;
	totalsize = download.totalsize;
	curkbps = download.curkbps;
	offs = download.offs;
	fn = download.fn.clone();
	return *this;
}

const newsboat::Filepath Download::filename() const
{
	return fn.clone();
}

const newsboat::Filepath Download::basename() const
{
	std::string::size_type start = fn.to_locale_string().rfind(NEWSBEUTER_PATH_SEP);

	if (start != std::string::npos) {
		return fn.to_locale_string().substr(start+1);
	}
	return fn.clone();
}

const std::string& Download::url() const
{
	return url_;
}

void Download::set_filename(const newsboat::Filepath& str)
{
	fn = str.clone();
}

double Download::percents_finished() const
{
	if (totalsize < 1) {
		return 0.0;
	} else {
		return (100 * (offs + cursize)) / (offs + totalsize);
	}
}

const std::string Download::status_text() const
{
	switch (download_status) {
	case DlStatus::QUEUED:
		return _s("queued");
	case DlStatus::DOWNLOADING:
		return _s("downloading");
	case DlStatus::CANCELLED:
		return _s("cancelled");
	case DlStatus::DELETED:
		return _s("deleted");
	case DlStatus::FINISHED:
		return _s("finished");
	case DlStatus::FAILED:
		return _s("failed");
	case DlStatus::MISSING:
		return _s("missing");
	case DlStatus::READY:
		return _s("ready");
	case DlStatus::PLAYED:
		return _s("played");
	case DlStatus::RENAME_FAILED:
		return _s("rename failed");
	default:
		return _s("unknown (bug).");
	}
}

void Download::set_url(const std::string& u)
{
	url_ = u;
}

void Download::set_progress(double downloaded, double total)
{
	if (downloaded > cursize) {
		cb_require_view_update();
	}
	cursize = downloaded;
	totalsize = total;
}

void Download::set_status(DlStatus dls, const std::string& msg_)
{
	if (download_status != dls) {
		cb_require_view_update();
	}
	msg = msg_;
	download_status = dls;
}

void Download::set_kbps(double k)
{
	curkbps = k;
}

double Download::kbps() const
{
	return curkbps;
}

void Download::set_offset(unsigned long offset)
{
	offs = offset;
}

} // namespace podboat

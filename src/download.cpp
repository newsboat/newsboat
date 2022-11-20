#include "download.h"

#include <string>

#include "config.h"
#include "pbcontroller.h"

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

Download::~Download() {}

const Utf8String Download::filename() const
{
	return fn;
}

const Utf8String Download::basename() const
{
	auto start = fn.rfind(NEWSBOAT_PATH_SEP);

	if (start != std::string::npos) {
		return fn.map([&](std::string s) {
			return s.substr(start+1);
		});
	}
	return fn;
}

const Utf8String Download::url() const
{
	return url_;
}

void Download::set_filename(const Utf8String& str)
{
	fn = str;
}

double Download::percents_finished() const
{
	if (totalsize < 1) {
		return 0.0;
	} else {
		return (100 * (offs + cursize)) / (offs + totalsize);
	}
}

const Utf8String Download::status_text() const
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
	case DlStatus::ALREADY_DOWNLOADED:
		return _s("incomplete");
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

void Download::set_url(const Utf8String& u)
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

void Download::set_status(DlStatus dls, const Utf8String& msg_)
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

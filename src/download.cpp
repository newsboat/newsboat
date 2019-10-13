#include "download.h"

#include <string>

#include "config.h"
#include "pbcontroller.h"

namespace podboat {

/*
 * the Download class represents a single download entry in podboat.
 * It manages the filename, the URL, the current state, the progress, etc.
 */

Download::Download(PbController* c)
	: download_status(DlStatus::QUEUED)
	, cursize(0.0)
	, totalsize(0.0)
	, curkbps(0.0)
	, offs(0)
	, ctrl(c)
{}

Download::~Download() {}

const std::string Download::filename() const
{
	return fn;
}

const std::string Download::basename() const
{
	std::string::size_type start = fn.rfind('/');

	if (start != std::string::npos) {
		return fn.substr(start + 1);
	}
	return fn;
}

const std::string Download::url() const
{
	return url_;
}

void Download::set_filename(const std::string& str)
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
	case DlStatus::ALREADY_DOWNLOADED:
		return _s("incomplete");
	case DlStatus::READY:
		return _s("ready");
	case DlStatus::PLAYED:
		return _s("played");
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
	if (downloaded > cursize)
		ctrl->set_view_update_necessary(true);
	cursize = downloaded;
	totalsize = total;
}

void Download::set_status(DlStatus dls)
{
	if (download_status != dls) {
		ctrl->set_view_update_necessary(true);
	}
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

#include "download.h"

#include <string>

#include "config.h"
#include "pb_controller.h"

namespace podboat {

/*
 * the download class represents a single download entry in podboat.
 * It manages the filename, the URL, the current state, the progress, etc.
 */

download::download(pb_controller* c)
	: download_status(dlstatus::QUEUED)
	, cursize(0.0)
	, totalsize(0.0)
	, curkbps(0.0)
	, offs(0)
	, ctrl(c)
{
}

download::~download() {}

const std::string download::filename()
{
	return fn;
}

const std::string download::url()
{
	return url_;
}

void download::set_filename(const std::string& str)
{
	fn = str;
}

double download::percents_finished()
{
	if (totalsize < 1) {
		return 0.0;
	} else {
		return (100 * (offs + cursize)) / (offs + totalsize);
	}
}

const std::string download::status_text()
{
	switch (download_status) {
	case dlstatus::QUEUED:
		return _s("queued");
	case dlstatus::DOWNLOADING:
		return _s("downloading");
	case dlstatus::CANCELLED:
		return _s("cancelled");
	case dlstatus::DELETED:
		return _s("deleted");
	case dlstatus::FINISHED:
		return _s("finished");
	case dlstatus::FAILED:
		return _s("failed");
	case dlstatus::ALREADY_DOWNLOADED:
		return _s("incomplete");
	case dlstatus::READY:
		return _s("ready");
	case dlstatus::PLAYED:
		return _s("played");
	default:
		return _s("unknown (bug).");
	}
}

void download::set_url(const std::string& u)
{
	url_ = u;
}

void download::set_progress(double downloaded, double total)
{
	if (downloaded > cursize)
		ctrl->set_view_update_necessary(true);
	cursize = downloaded;
	totalsize = total;
}

void download::set_status(dlstatus dls)
{
	if (download_status != dls) {
		ctrl->set_view_update_necessary(true);
	}
	download_status = dls;
}

void download::set_kbps(double k)
{
	curkbps = k;
}

double download::kbps()
{
	return curkbps;
}

void download::set_offset(unsigned long offset)
{
	offs = offset;
}

} // namespace podboat

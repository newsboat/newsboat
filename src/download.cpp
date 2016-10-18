#include <download.h>
#include <pb_controller.h>
#include <config.h>
#include <string>

namespace podbeuter {

/*
 * the download class represents a single download entry in podbeuter.
 * It manages the filename, the URL, the current state, the progress, etc.
 */

download::download(pb_controller * c)
: download_status(dlstatus::QUEUED), cursize(0.0), totalsize(0.0), curkbps(0.0),
	offs(0), ctrl(c)
{ }

download::~download() {
}

const char * download::filename() {
	return fn.c_str();
}

const char * download::url() {
	return url_.c_str();
}

void download::set_filename(const std::string& str) {
	fn = str;
}

double download::percents_finished() {
	if (totalsize < 1) {
		return 0.0;
	} else {
		return (100*(offs + cursize))/(offs + totalsize);
	}
}

const char * download::status_text() {
	switch (download_status) {
	case dlstatus::QUEUED:
		return _("queued");
	case dlstatus::DOWNLOADING:
		return _("downloading");
	case dlstatus::CANCELLED:
		return _("cancelled");
	case dlstatus::DELETED:
		return _("deleted");
	case dlstatus::FINISHED:
		return _("finished");
	case dlstatus::FAILED:
		return _("failed");
	case dlstatus::ALREADY_DOWNLOADED:
		return _("incomplete");
	case dlstatus::READY:
		return _("ready");
	case dlstatus::PLAYED:
		return _("played");
	default:
		return _("unknown (bug).");
	}
}

void download::set_url(const std::string& u) {
	url_ = u;
}

void download::set_progress(double cur, double max) {
	if (cur > cursize)
		ctrl->set_view_update_necessary(true);
	cursize = cur;
	totalsize = max;
}

void download::set_status(dlstatus dls) {
	if (download_status != dls) {
		ctrl->set_view_update_necessary(true);
	}
	download_status = dls;
}

void download::set_kbps(double k) {
	curkbps = k;
}

double download::kbps() {
	return curkbps;
}

void download::set_offset(unsigned long offset) {
	offs = offset;
}

}

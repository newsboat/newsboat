#include <download.h>
#include <pb_controller.h>
#include <config.h>
#include <string>

namespace podbeuter {

/*
 * the download class represents a single download entry in podbeuter.
 * It manages the filename, the URL, the current state, the progress, etc.
 */

download::download(pb_controller * c) : dlstatus(DL_QUEUED), cursize(0.0), totalsize(0.0), curkbps(0.0), offs(0), ctrl(c) {
}

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
	switch (dlstatus) {
		case DL_QUEUED:
			return _("queued");
		case DL_DOWNLOADING:
			return _("downloading");
		case DL_CANCELLED:
			return _("cancelled");
		case DL_DELETED:
			return _("deleted");
		case DL_FINISHED:
			return _("finished");
		case DL_FAILED:
			return _("failed");
		case DL_ALREADY_DOWNLOADED:
			return _("incomplete");
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

void download::set_status(dlstatus_t dls) {
	if (dlstatus != dls) {
		ctrl->set_view_update_necessary(true);
	}
	dlstatus = dls;
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

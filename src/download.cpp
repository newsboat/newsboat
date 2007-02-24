#include <download.h>
#include <pb_controller.h>
#include <config.h>
#include <string>

namespace podbeuter {

download::download(pb_controller * c) : dlstatus(DL_QUEUED), cursize(0), totalsize(0), ctrl(c) {
}

download::~download() {
}

const char * download::filename() {
	return fn.c_str();
}

void download::set_filename(const std::string& str) {
	fn = str;
}

float download::percents_finished() {
	if (totalsize < 1) {
		return 0.0;
	} else {
		return (100*cursize)/totalsize;
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
		default:
			return _("unknown (bug).");
	}
}

void download::set_url(const std::string& url) {
	this->url = url;
}

void download::set_progress(float cur, float max) {
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

}

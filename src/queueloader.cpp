#include <stflpp.h>
#include <utils.h>
#include <queueloader.h>
#include <cstdlib>
#include <logger.h>
#include <fstream>
#include <cstring>
#include <config.h>
#include <libgen.h>

#include <unistd.h>

using namespace newsbeuter;

namespace podbeuter {

queueloader::queueloader(const std::string& file, pb_controller * c) : queuefile(file), ctrl(c) {
}

void queueloader::reload(std::vector<download>& downloads, bool remove_unplayed) {
	std::vector<download> dltemp;
	std::fstream f;

	if (downloads.size() > 0) {
		for (std::vector<download>::iterator it=downloads.begin();it!=downloads.end();++it) {
			if (it->status() == DL_DOWNLOADING) { // we are not allowed to reload if a download is in progress!
				LOG(LOG_INFO, "queueloader::reload: aborting reload due to DL_DOWNLOADING status");
				return;
			}
			switch (it->status()) {
				case DL_QUEUED:
				case DL_CANCELLED:
				case DL_FAILED:
				case DL_ALREADY_DOWNLOADED:
				case DL_READY:
					LOG(LOG_DEBUG, "queueloader::reload: storing %s to new vector", it->url());
					dltemp.push_back(*it);
					break;
				case DL_PLAYED:
				case DL_FINISHED:
					if (!remove_unplayed) {
						LOG(LOG_DEBUG, "queueloader::reload: storing %s to new vector", it->url());
						dltemp.push_back(*it);
					}
					break;
				default:
					break;
			}
		}
	}

	f.open(queuefile.c_str(), std::fstream::in);
	if (f.is_open()) {
		std::string line;
		do {
			getline(f, line);
			if (!f.eof() && line.length() > 0) {
				LOG(LOG_DEBUG, "queueloader::reload: loaded `%s' from queue file", line.c_str());
				std::vector<std::string> fields = utils::tokenize_quoted(line);
				bool url_found = false;
				if (!dltemp.empty()) {
					for (std::vector<download>::iterator it=dltemp.begin();it!=dltemp.end();++it) {
						if (fields[0] == it->url()) {
							LOG(LOG_INFO, "queueloader::reload: found `%s' in old vector", fields[0].c_str());
							url_found = true;
							break;
						}
					}
				}
				if (!downloads.empty()) {
					for (std::vector<download>::iterator it=downloads.begin();it!=downloads.end();++it) {
						if (fields[0] == it->url()) {
							LOG(LOG_INFO, "queueloader::reload: found `%s' in new vector", line.c_str());
							url_found = true;
							break;
						}
					}
				}
				if (!url_found) {
					LOG(LOG_INFO, "queueloader::reload: found `%s' nowhere -> storing to new vector", line.c_str());
					download d(ctrl);
					std::string fn;
					if (fields.size() == 1)
						fn = get_filename(fields[0]);
					else
						fn = fields[1];
					d.set_filename(fn);
					if (access(fn.c_str(), F_OK)==0) {
						LOG(LOG_INFO, "queueloader::reload: found `%s' on file system -> mark as already downloaded", fn.c_str());
						if (fields.size() >= 3) {
							if (fields[2] == "downloaded")
								d.set_status(DL_READY);
							if (fields[2] == "played")
								d.set_status(DL_PLAYED);
						}
						else
							d.set_status(DL_ALREADY_DOWNLOADED); // TODO: scrap DL_ALREADY_DOWNLOADED state
					}
					d.set_url(fields[0]);
					dltemp.push_back(d);
				}
			}
		} while (!f.eof());
		f.close();
	}

	f.open(queuefile.c_str(), std::fstream::out);
	if (f.is_open()) {
		for (std::vector<download>::iterator it=dltemp.begin();it!=dltemp.end();++it) {
			f << it->url() << " " << stfl::quote(it->filename());
			if (it->status() == DL_READY)
				f << " downloaded";
			if (it->status() == DL_PLAYED)
				f << " played";
			f << std::endl;
		}
		f.close();
	}

	downloads = dltemp;
}

std::string queueloader::get_filename(const std::string& str) {
	std::string fn = ctrl->get_dlpath();

	if (fn[fn.length()-1] != NEWSBEUTER_PATH_SEP[0])
		fn.append(NEWSBEUTER_PATH_SEP);
	char buf[1024];
	snprintf(buf, sizeof(buf), "%s", str.c_str());
	char * base = basename(buf);
	if (!base || strlen(base) == 0) {
		char lbuf[128];
		time_t t = time(NULL);
		strftime(lbuf, sizeof(lbuf), "%Y-%b-%d-%H%M%S.unknown", localtime(&t));
		fn.append(lbuf);
	} else {
		fn.append(base);
	}
	return fn;
}

}

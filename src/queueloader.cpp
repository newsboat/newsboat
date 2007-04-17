#include <queueloader.h>
#include <cstdlib>
#include <logger.h>
#include <fstream>
#include <config.h>
#include <libgen.h>

using namespace newsbeuter;

namespace podbeuter {

queueloader::queueloader(const std::string& file, pb_controller * c) : queuefile(file), ctrl(c) {
}

void queueloader::reload(std::vector<download>& downloads) {
	std::vector<download> dltemp;
	std::fstream f;

	if (downloads.size() > 0) {
		for (std::vector<download>::iterator it=downloads.begin();it!=downloads.end();++it) {
			if (it->status() == DL_DOWNLOADING) { // we are not allowed to reload if a download is in progress!
				GetLogger().log(LOG_INFO, "queueloader::reload: aborting reload due to DL_DOWNLOADING status");
				return;
			}
			if (it->status() == DL_QUEUED || it->status() == DL_CANCELLED || it->status() == DL_FAILED) {
				GetLogger().log(LOG_DEBUG, "queueloader::reload: storing %s to new vector", it->url());
				dltemp.push_back(*it);
			}
		}
	}

	f.open(queuefile.c_str(), std::fstream::in);
	if (f.is_open()) {
		std::string line;
		do {
			getline(f, line);
			if (!f.eof() && line.length() > 0) {
				GetLogger().log(LOG_DEBUG, "queueloader::reload: loaded `%s' from queue file", line.c_str());
				bool url_found = false;
				if (dltemp.size() > 0) {
					for (std::vector<download>::iterator it=dltemp.begin();it!=dltemp.end();++it) {
						if (line == it->url()) {
							GetLogger().log(LOG_INFO, "queueloader::reload: found `%s' in old vector", line.c_str());
							url_found = true;
							break;
						}
					}
				}
				if (downloads.size() > 0) {
					for (std::vector<download>::iterator it=downloads.begin();it!=downloads.end();++it) {
						if (line == it->url()) {
							GetLogger().log(LOG_INFO, "queueloader::reload: found `%s' in new vector", line.c_str());
							url_found = true;
							break;
						}
					}
				}
				if (!url_found) {
					GetLogger().log(LOG_INFO, "queueloader::reload: found `%s' nowhere -> storing to new vector", line.c_str());
					download d(ctrl);
					std::string fn = get_filename(line);
					d.set_filename(fn);
					if (access(fn.c_str(), F_OK)==0) {
						GetLogger().log(LOG_INFO, "queueloader::reload: found `%s' on file system -> mark as already downloaded", fn.c_str());
						d.set_status(DL_ALREADY_DOWNLOADED); // TODO: scrap DL_ALREADY_DOWNLOADED state
					}
					d.set_url(line);
					dltemp.push_back(d);
				}
			}
		} while (!f.eof());
		f.close();
	}

	f.open(queuefile.c_str(), std::fstream::out);
	if (f.is_open()) {
		for (std::vector<download>::iterator it=dltemp.begin();it!=dltemp.end();++it) {
			f << it->url() << std::endl;
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
		char buf[128];
		time_t t = time(NULL);
		strftime(buf, sizeof(buf), "%Y-%b-%d-%H%M%S.unknown", localtime(&t));
		fn.append(buf);
	} else {
		fn.append(base);
	}
	return fn;
}

}

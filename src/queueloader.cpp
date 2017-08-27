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

	for (auto dl : downloads) {
		if (dl.status() == dlstatus::DOWNLOADING) { // we are not allowed to reload if a download is in progress!
			LOG(level::INFO, "queueloader::reload: aborting reload due to dlstatus::DOWNLOADING status");
			return;
		}
		switch (dl.status()) {
		case dlstatus::QUEUED:
		case dlstatus::CANCELLED:
		case dlstatus::FAILED:
		case dlstatus::ALREADY_DOWNLOADED:
		case dlstatus::READY:
			LOG(level::DEBUG, "queueloader::reload: storing %s to new vector", dl.url());
			dltemp.push_back(dl);
			break;
		case dlstatus::PLAYED:
		case dlstatus::FINISHED:
			if (!remove_unplayed) {
				LOG(level::DEBUG, "queueloader::reload: storing %s to new vector", dl.url());
				dltemp.push_back(dl);
			}
			break;
		default:
			break;
		}
	}

	f.open(queuefile.c_str(), std::fstream::in);
	if (f.is_open()) {
		std::string line;
		do {
			std::getline(f, line);
			if (!f.eof() && line.length() > 0) {
				LOG(level::DEBUG, "queueloader::reload: loaded `%s' from queue file", line);
				std::vector<std::string> fields = utils::tokenize_quoted(line);
				bool url_found = false;

				for (auto dl : dltemp) {
					if (fields[0] == dl.url()) {
						LOG(level::INFO, "queueloader::reload: found `%s' in old vector", fields[0]);
						url_found = true;
						break;
					}
				}

				for (auto dl : downloads) {
					if (fields[0] == dl.url()) {
						LOG(level::INFO, "queueloader::reload: found `%s' in new vector", line);
						url_found = true;
						break;
					}
				}

				if (!url_found) {
					LOG(level::INFO, "queueloader::reload: found `%s' nowhere -> storing to new vector", line);
					download d(ctrl);
					std::string fn;
					if (fields.size() == 1)
						fn = get_filename(fields[0]);
					else
						fn = fields[1];
					d.set_filename(fn);
					if (access(fn.c_str(), F_OK)==0) {
						LOG(level::INFO, "queueloader::reload: found `%s' on file system -> mark as already downloaded", fn);
						if (fields.size() >= 3) {
							if (fields[2] == "downloaded")
								d.set_status(dlstatus::READY);
							if (fields[2] == "played")
								d.set_status(dlstatus::PLAYED);
						} else
							d.set_status(dlstatus::ALREADY_DOWNLOADED); // TODO: scrap dlstatus::ALREADY_DOWNLOADED state
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
		for (auto dl : dltemp) {
			f << dl.url() << " " << stfl::quote(dl.filename());
			if (dl.status() == dlstatus::READY)
				f << " downloaded";
			if (dl.status() == dlstatus::PLAYED)
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
		time_t t = time(nullptr);
		strftime(lbuf, sizeof(lbuf), "%Y-%b-%d-%H%M%S.unknown", localtime(&t));
		fn.append(lbuf);
	} else {
		fn.append(utils::replace_all(base, "'", "%27"));
	}
	return fn;
}

}

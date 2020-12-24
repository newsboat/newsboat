#include "queueloader.h"

#include <cerrno>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <libgen.h>
#include <unistd.h>

#include "config.h"
#include "configcontainer.h"
#include "logger.h"
#include "stflpp.h"
#include "strprintf.h"
#include "utils.h"

using namespace newsboat;

namespace podboat {

QueueLoader::QueueLoader(const std::string& file, ConfigContainer& cfg_,
	std::function<void()> cb_require_view_update_)
	: queuefile(file)
	, cfg(cfg_)
	, cb_require_view_update(cb_require_view_update_)
{
}

void QueueLoader::reload(std::vector<Download>& downloads,
	bool also_remove_finished) const
{
	std::vector<Download> dltemp;
	std::vector<Download> deletion_list;
	for (const auto& dl : downloads) {
		// we are not allowed to reload if a download is in progress!
		if (dl.status() == DlStatus::DOWNLOADING) {
			LOG(Level::INFO,
				"QueueLoader::reload: aborting reload due to "
				"DlStatus::DOWNLOADING status");
			return;
		}
		bool keep_entry = false;
		switch (dl.status()) {
		case DlStatus::QUEUED:
		case DlStatus::CANCELLED:
		case DlStatus::FAILED:
		case DlStatus::ALREADY_DOWNLOADED:
		case DlStatus::READY:
		case DlStatus::PLAYED:
			LOG(Level::DEBUG,
				"QueueLoader::reload: storing %s to new vector",
				dl.url());
			keep_entry = true;
			break;
		case DlStatus::FINISHED:
			if (!also_remove_finished) {
				LOG(Level::DEBUG,
					"QueueLoader::reload: storing %s to "
					"new "
					"vector",
					dl.url());
				keep_entry = true;
			}
			break;
		default:
			break;
		}

		if (keep_entry) {
			dltemp.push_back(dl);
		} else {
			deletion_list.push_back(dl);
		}
	}

	std::fstream f(queuefile, std::fstream::in);
	bool comments_ignored = false;
	if (f.is_open()) {
		std::string line;
		do {
			std::getline(f, line);
			if (!f.eof() && line.length() > 0) {
				LOG(Level::DEBUG,
					"QueueLoader::reload: loaded `%s' from "
					"queue file",
					line);
				std::vector<std::string> fields =
					utils::tokenize_quoted(line);
				bool url_found = false;

				if (fields.empty()) {
					if (!comments_ignored) {
						std::cout << strprintf::fmt(
								_("WARNING: Comment found "
									"in %s. The queue file is regenerated "
									"when podboat exits and comments will "
									"be deleted. Press Enter to continue or "
									"Ctrl+C to abort"),
								queuefile)
							<< std::endl;
						std::cin.ignore();
						comments_ignored = true;
					}
					continue;
				}

				for (const auto& dl : dltemp) {
					if (fields[0] == dl.url()) {
						LOG(Level::INFO,
							"QueueLoader::reload: "
							"found `%s' in old "
							"vector",
							fields[0]);
						url_found = true;
						break;
					}
				}

				for (const auto& dl : deletion_list) {
					if (fields[0] == dl.url()) {
						LOG(Level::INFO,
							"QueueLoader::reload: "
							"found `%s' in scheduled for deletion "
							"vector",
							fields[0]);
						url_found = true;
						break;
					}
				}

				if (!url_found) {
					LOG(Level::INFO,
						"QueueLoader::reload: found "
						"`%s' "
						"nowhere -> storing to new "
						"vector",
						line);
					Download d(cb_require_view_update);
					std::string fn;
					if (fields.size() == 1) {
						fn = get_filename(fields[0]);
					} else {
						fn = fields[1];
					}
					d.set_filename(fn);
					if (access(fn.c_str(), F_OK) == 0) {
						LOG(Level::INFO,
							"QueueLoader::reload: "
							"found `%s' on file "
							"system "
							"-> mark as already "
							"downloaded",
							fn);
						if (fields.size() >= 3) {
							if (fields[2] ==
								"downloaded")
								d.set_status(
									DlStatus::
									READY);
							if (fields[2] ==
								"played")
								d.set_status(
									DlStatus::
									PLAYED);
							if (fields[2] ==
								"finished")
								d.set_status(
									DlStatus::
									FINISHED);
						} else
							d.set_status(DlStatus::
								ALREADY_DOWNLOADED); // TODO: scrap DlStatus::ALREADY_DOWNLOADED state
					} else if (
						access((fn +
								ConfigContainer::
								PARTIAL_FILE_SUFFIX)
							.c_str(),
							F_OK) == 0) {
						LOG(Level::INFO,
							"QueueLoader::reload: "
							"found `%s' on file "
							"system "
							"-> mark as partially "
							"downloaded",
							fn +
							ConfigContainer::
							PARTIAL_FILE_SUFFIX);
						d.set_status(DlStatus::
							ALREADY_DOWNLOADED);
					}

					d.set_url(fields[0]);
					dltemp.push_back(d);
				}
			}
		} while (!f.eof());
		f.close();
	}

	f.open(queuefile, std::fstream::out);
	if (f.is_open()) {
		for (const auto& dl : dltemp) {
			f << dl.url() << " " << utils::quote(dl.filename());
			if (dl.status() == DlStatus::READY) {
				f << " downloaded";
			}
			if (dl.status() == DlStatus::PLAYED) {
				f << " played";
			}
			if (dl.status() == DlStatus::FINISHED) {
				f << " finished";
			}
			f << std::endl;
		}
		f.close();
	}

	if (cfg.get_configvalue_as_bool("delete-played-files")) {
		for (const auto& dl : deletion_list) {
			const std::string filename = dl.filename();
			LOG(Level::INFO,
				"Deleting file %s",
				filename);
			if (std::remove(filename.c_str()) != 0) {
				if (errno != ENOENT) {
					LOG(Level::ERROR,
						"Failed to delete file %s, error code: %d (%s)",
						filename, errno, strerror(errno));
				}
			}
		}
	}

	downloads = dltemp;
}

std::string QueueLoader::get_filename(const std::string& str) const
{
	std::string fn = cfg.get_configvalue("download-path");

	if (fn[fn.length() - 1] != NEWSBEUTER_PATH_SEP) {
		fn.push_back(NEWSBEUTER_PATH_SEP);
	}
	char buf[1024];
	snprintf(buf, sizeof(buf), "%s", str.c_str());
	char* base = basename(buf);
	if (!base || strlen(base) == 0) {
		time_t t = time(nullptr);
		fn.append(utils::mt_strf_localtime("%Y-%b-%d-%H%M%S.unknown", t));
	} else {
		fn.append(utils::replace_all(base, "'", "%27"));
	}
	return fn;
}

} // namespace podboat

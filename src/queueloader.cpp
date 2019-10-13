#include "queueloader.h"

#include <cstdlib>
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

QueueLoader::QueueLoader(const std::string& file, PbController* c)
	: queuefile(file)
	, ctrl(c)
{}

void QueueLoader::reload(std::vector<Download>& downloads, bool remove_unplayed)
{
	std::vector<Download> dltemp;
	std::fstream f;

	for (const auto& dl : downloads) {
		if (dl.status() == DlStatus::DOWNLOADING) { // we are not
							    // allowed to reload
							    // if a download is
							    // in progress!
			LOG(Level::INFO,
				"QueueLoader::reload: aborting reload due to "
				"DlStatus::DOWNLOADING status");
			return;
		}
		switch (dl.status()) {
		case DlStatus::QUEUED:
		case DlStatus::CANCELLED:
		case DlStatus::FAILED:
		case DlStatus::ALREADY_DOWNLOADED:
		case DlStatus::READY:
			LOG(Level::DEBUG,
				"QueueLoader::reload: storing %s to new vector",
				dl.url());
			dltemp.push_back(dl);
			break;
		case DlStatus::PLAYED:
		case DlStatus::FINISHED:
			if (!remove_unplayed) {
				LOG(Level::DEBUG,
					"QueueLoader::reload: storing %s to "
					"new "
					"vector",
					dl.url());
				dltemp.push_back(dl);
			}
			break;
		default:
			break;
		}
	}

	f.open(queuefile.c_str(), std::fstream::in);
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
						std::cout
							<< strprintf::fmt(
								   _("WARNING: "
								     "Comment "
								     "found "
								     "in %s. "
								     "The "
								     "queue "
								     "file is "
								     "regenerat"
								     "ed "
								     "when "
								     "podboat "
								     "exits "
								     "and "
								     "comments "
								     "will "
								     "be "
								     "deleted. "
								     "Press "
								     "enter to "
								     "continue "
								     "or "
								     "Ctrl+C "
								     "to "
								     "abort"),
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

				for (const auto& dl : downloads) {
					if (fields[0] == dl.url()) {
						LOG(Level::INFO,
							"QueueLoader::reload: "
							"found `%s' in new "
							"vector",
							line);
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
					Download d(ctrl);
					std::string fn;
					if (fields.size() == 1)
						fn = get_filename(fields[0]);
					else
						fn = fields[1];
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

	f.open(queuefile.c_str(), std::fstream::out);
	if (f.is_open()) {
		for (const auto& dl : dltemp) {
			f << dl.url() << " " << utils::quote(dl.filename());
			if (dl.status() == DlStatus::READY)
				f << " downloaded";
			if (dl.status() == DlStatus::PLAYED)
				f << " played";
			f << std::endl;
		}
		f.close();
	}

	downloads = dltemp;
}

std::string QueueLoader::get_filename(const std::string& str)
{
	std::string fn = ctrl->get_dlpath();

	if (fn[fn.length() - 1] != NEWSBEUTER_PATH_SEP[0])
		fn.append(NEWSBEUTER_PATH_SEP);
	char buf[1024];
	snprintf(buf, sizeof(buf), "%s", str.c_str());
	char* base = basename(buf);
	if (!base || strlen(base) == 0) {
		char lbuf[128];
		time_t t = time(nullptr);
		strftime(lbuf,
			sizeof(lbuf),
			"%Y-%b-%d-%H%M%S.unknown",
			localtime(&t));
		fn.append(lbuf);
	} else {
		fn.append(utils::replace_all(base, "'", "%27"));
	}
	return fn;
}

} // namespace podboat

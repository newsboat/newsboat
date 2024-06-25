#define ENABLE_IMPLICIT_FILEPATH_CONVERSIONS

#include "queueloader.h"

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <libgen.h>
#include <unistd.h>

#include "config.h"
#include "configcontainer.h"
#include "logger.h"
#include "strprintf.h"
#include "utils.h"

using namespace newsboat;

namespace podboat {

QueueLoader::QueueLoader(const Filepath& filepath,
	const ConfigContainer& cfg_,
	std::function<void()> cb_require_view_update_)
	: queuefile(filepath)
	, cfg(cfg_)
	, cb_require_view_update(cb_require_view_update_)
{
}

void QueueLoader::reload(std::vector<Download>& downloads,
	bool also_remove_finished) const
{
	CategorizedDownloads categorized_downloads;
	const auto res = categorize_downloads(downloads, also_remove_finished);
	if (!res.has_value()) {
		return;
	}
	categorized_downloads = res.value();

	update_from_queue_file(categorized_downloads);
	write_queue_file(categorized_downloads);
	if (cfg.get_configvalue_as_bool("delete-played-files")) {
		delete_played_files(categorized_downloads);
	}
	downloads = std::move(categorized_downloads.to_keep);
}

std::optional<QueueLoader::CategorizedDownloads> QueueLoader::categorize_downloads(
	const std::vector<Download>& downloads,
	bool also_remove_finished)
{
	CategorizedDownloads result;

	for (const auto& dl : downloads) {
		// we are not allowed to reload if a download is in progress!
		if (dl.status() == DlStatus::DOWNLOADING) {
			LOG(Level::INFO,
				"QueueLoader::reload: aborting reload due to "
				"DlStatus::DOWNLOADING status");
			return std::nullopt;
		}
		bool keep_entry = false;
		switch (dl.status()) {
		case DlStatus::QUEUED:
		case DlStatus::CANCELLED:
		case DlStatus::FAILED:
		case DlStatus::MISSING:
		case DlStatus::READY:
		case DlStatus::PLAYED:
		case DlStatus::RENAME_FAILED:
			LOG(Level::DEBUG,
				"QueueLoader::reload: storing %s to new vector",
				dl.url());
			keep_entry = true;
			break;
		case DlStatus::FINISHED:
			if (!also_remove_finished) {
				LOG(Level::DEBUG,
					"QueueLoader::reload: storing %s to new vector",
					dl.url());
				keep_entry = true;
			}
			break;
		case DlStatus::DELETED:
			keep_entry = false;
			break;

		case DlStatus::DOWNLOADING:
			assert(!"Can't be reached because of the `if` above");
			break;
		}

		if (keep_entry) {
			result.to_keep.push_back(dl);
		} else {
			result.to_delete.push_back(dl);
		}
	}

	return result;
}

void QueueLoader::update_from_queue_file(CategorizedDownloads& downloads) const
{
	std::fstream f(queuefile, std::fstream::in);
	if (!f.is_open()) {
		return;
	}

	bool comments_ignored = false;
	for (std::string line; std::getline(f, line); ) {
		if (line.empty()) {
			continue;
		}

		LOG(Level::DEBUG,
			"QueueLoader::reload: loaded `%s' from queue file",
			line);
		const std::vector<std::string> fields = utils::tokenize_quoted(line);
		bool url_found = false;

		if (fields.empty()) {
			if (!comments_ignored) {
				std::cout << strprintf::fmt(
						_("WARNING: Comment found "
							"in %s. The queue file is regenerated "
							"when Podboat exits and comments will "
							"be deleted. Press Enter to continue or "
							"Ctrl+C to abort"),
						queuefile)
					<< std::endl;
				std::cin.ignore();
				comments_ignored = true;
			}
			continue;
		}

		for (const auto& dl : downloads.to_keep) {
			if (fields[0] == dl.url()) {
				LOG(Level::INFO,
					"QueueLoader::reload: found `%s' in old vector",
					fields[0]);
				url_found = true;
				break;
			}
		}

		for (const auto& dl : downloads.to_delete) {
			if (fields[0] == dl.url()) {
				LOG(Level::INFO,
					"QueueLoader::reload: found `%s' in scheduled for deletion vector",
					fields[0]);
				url_found = true;
				break;
			}
		}

		if (url_found) {
			continue;
		}

		LOG(Level::INFO,
			"QueueLoader::reload: found `%s' nowhere -> storing to new vector",
			line);
		Download d(cb_require_view_update);
		std::string fn;
		if (fields.size() == 1) {
			fn = get_filename(fields[0]);
		} else {
			fn = fields[1];
		}
		d.set_filename(fn);

		if (fields.size() >= 3) {
			if (fields[2] == "missing") {
				d.set_status(DlStatus::MISSING);
			}
			if (fields[2] == "downloaded") {
				d.set_status(DlStatus::READY);
			}
			if (fields[2] == "played") {
				d.set_status(DlStatus::PLAYED);
			}
			if (fields[2] == "finished") {
				d.set_status(DlStatus::FINISHED);
			}
		}

		if (access(fn.c_str(), F_OK) == 0) {
			LOG(Level::INFO,
				"QueueLoader::reload: found `%s' on file system -> mark as already downloaded",
				fn);
			if (d.status() == DlStatus::QUEUED || d.status() == DlStatus::MISSING) {
				d.set_status(DlStatus::READY);
			}
		} else if (access((fn + ConfigContainer::PARTIAL_FILE_SUFFIX).c_str(),
				F_OK) == 0) {
			LOG(Level::INFO,
				"QueueLoader::reload: found `%s' on file system -> mark as partially downloaded",
				fn + ConfigContainer::PARTIAL_FILE_SUFFIX);
			d.set_status(DlStatus::FAILED);
		} else {
			if (d.status() != DlStatus::QUEUED) {
				d.set_status(DlStatus::MISSING);
			}
		}

		d.set_url(fields[0]);
		downloads.to_keep.push_back(d);
	}
}

void QueueLoader::write_queue_file(const CategorizedDownloads& downloads) const
{
	std::fstream f(queuefile, std::fstream::out);
	if (!f.is_open()) {
		return;
	}

	for (const auto& dl : downloads.to_keep) {
		f << dl.url() << " " << utils::quote(dl.filename());
		switch (dl.status()) {
		case DlStatus::READY:
			f << " downloaded";
			break;

		case DlStatus::PLAYED:
			f << " played";
			break;

		case DlStatus::FINISHED:
			f << " finished";
			break;

		case DlStatus::MISSING:
			f << " missing";
			break;

		// The following statuses have no marks in the queue file.
		case DlStatus::QUEUED:
		case DlStatus::DOWNLOADING:
		case DlStatus::CANCELLED:
		case DlStatus::DELETED:
		case DlStatus::FAILED:
		case DlStatus::RENAME_FAILED:
			break;
		}
		f << std::endl;
	}
}

void QueueLoader::delete_played_files(const CategorizedDownloads& downloads)
const
{
	for (const auto& dl : downloads.to_delete) {
		const Filepath filename = dl.filename();
		LOG(Level::INFO, "Deleting file %s", filename);
		if (std::remove(filename.to_locale_string().c_str()) != 0) {
			if (errno != ENOENT) {
				LOG(Level::ERROR,
					"Failed to delete file %s, error code: %d (%s)",
					filename, errno, strerror(errno));
			}
		}
	}
}

newsboat::Filepath QueueLoader::get_filename(const std::string& str) const
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

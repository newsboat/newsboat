#ifndef PODBOAT_QUEUELOADER_H_
#define PODBOAT_QUEUELOADER_H_

#include <functional>
#include <optional>
#include <vector>

#include "configcontainer.h"
#include "download.h"
#include "filepath.h"

namespace podboat {

/// Synchronizes Podboat's array of downloads with the queue file on the
/// filesystem.
class QueueLoader {
public:
	/// Create a loader that will work with the queue file at \a filepath.
	/// `Download` objects will be given \a cb_require_view_update as an
	/// argument.
	QueueLoader(const newsboat::Filepath& filepath, const newsboat::ConfigContainer& cfg,
		std::function<void()> cb_require_view_update);

	/// Synchronize the queue file with \a downloads. Downloads with `DELETED`
	/// status are removed. If \a also_remove_finished is `true`, `FINISHED`
	/// downloads are removed too.
	void reload(std::vector<Download>& downloads,
		bool also_remove_finished = false) const;

private:
	newsboat::Filepath get_filename(const std::string& str) const;

	const newsboat::Filepath queuefile;
	const newsboat::ConfigContainer& cfg;
	std::function<void()> cb_require_view_update;

	/// A helper type for methods that process the queue file.
	struct CategorizedDownloads {
		/// Downloads that should be kept in the queue file.
		std::vector<Download> to_keep;

		/// Downloads that should be deleted from the queue file.
		std::vector<Download> to_delete;
	};

	/// Splits downloads into "to keep" and "to delete" categories depending on
	/// their status.
	///
	/// If `also_remove_finished` is `true`, downloads with `FINISHED` status
	/// are put into "to delete" category.
	///
	/// Returns:
	/// - nullopt if one of the downloads is currently being downloaded;
	/// - otherwise, a struct with categorized downloads.
	static std::optional<CategorizedDownloads> categorize_downloads(
		const std::vector<Download>& downloads, bool also_remove_finished);

	/// Adds downloads from the queue file to the "to keep" category.
	void update_from_queue_file(CategorizedDownloads& downloads) const;

	/// Writes "to keep" downloads to the queue file.
	void write_queue_file(const CategorizedDownloads& downloads) const;

	/// If `delete-played-files` is enabled, deletes downloaded files
	/// corresponding to downloads in the "to delete" category.
	void delete_played_files(const CategorizedDownloads& downloads) const;
};

} // namespace podboat

#endif /* PODBOAT_QUEUELOADER_H_ */

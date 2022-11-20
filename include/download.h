#ifndef PODBOAT_DOWNLOAD_H_
#define PODBOAT_DOWNLOAD_H_

#include <functional>

#include "utf8string.h"

using newsboat::Utf8String;

namespace podboat {

enum class DlStatus {
	QUEUED = 0,
	DOWNLOADING,
	CANCELLED,
	DELETED,
	FINISHED,
	FAILED,
	ALREADY_DOWNLOADED,
	READY,
	PLAYED,
	RENAME_FAILED
};

class Download {
public:
	explicit Download(std::function<void()> cb_require_view_update);
	~Download();
	double percents_finished() const;
	const Utf8String status_text() const;
	DlStatus status() const
	{
		return download_status;
	}
	const Utf8String& status_msg() const
	{
		return msg;
	}
	const Utf8String filename() const;
	const Utf8String basename() const;
	const Utf8String url() const;
	void set_filename(const Utf8String& str);
	void set_url(const Utf8String& url);
	void set_progress(double downloaded, double total);
	void set_status(DlStatus dls, const Utf8String& msg_ = {});
	void set_kbps(double kbps);
	double kbps() const;
	void set_offset(unsigned long offset);

	double current_size() const
	{
		return cursize + offs;
	}
	double total_size() const
	{
		return totalsize + offs;
	}

private:
	Utf8String fn;
	Utf8String url_;
	DlStatus download_status;
	Utf8String msg;
	double cursize;
	double totalsize;
	double curkbps;
	unsigned long offs;
	std::function<void()> cb_require_view_update;
};

} // namespace podboat

#endif /* PODBOAT_DOWNLOAD_H_ */

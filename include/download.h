#ifndef PODBOAT_DOWNLOAD_H_
#define PODBOAT_DOWNLOAD_H_

#include <functional>
#include <string>

namespace podboat {

enum class DlStatus {
	QUEUED = 0,
	DOWNLOADING,
	CANCELLED,
	DELETED,
	FINISHED,
	FAILED,
	READY,
	PLAYED,
	RENAME_FAILED,
	MISSING,
};

class Download {
public:
	explicit Download(std::function<void()> cb_require_view_update);
	double percents_finished() const;
	const std::string status_text() const;
	DlStatus status() const
	{
		return download_status;
	}
	const std::string& status_msg() const
	{
		return msg;
	}
	const std::string& filename() const;
	const std::string basename() const;
	const std::string& url() const;
	void set_filename(const std::string& str);
	void set_url(const std::string& url);
	void set_progress(double downloaded, double total);
	void set_status(DlStatus dls, const std::string& msg_ = {});
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
	std::string fn;
	std::string url_;
	DlStatus download_status;
	std::string msg;
	double cursize;
	double totalsize;
	double curkbps;
	unsigned long offs;
	std::function<void()> cb_require_view_update;
};

} // namespace podboat

#endif /* PODBOAT_DOWNLOAD_H_ */

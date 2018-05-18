#ifndef PODBOAT_DOWNLOAD_H_
#define PODBOAT_DOWNLOAD_H_

#include <string>

namespace podboat {

enum class dlstatus {
	QUEUED = 0,
	DOWNLOADING,
	CANCELLED,
	DELETED,
	FINISHED,
	FAILED,
	ALREADY_DOWNLOADED,
	READY,
	PLAYED
};

class pb_controller;

class download {
public:
	explicit download(pb_controller* c = 0);
	~download();
	double percents_finished();
	const std::string status_text();
	dlstatus status() const
	{
		return download_status;
	}
	const std::string filename();
	const std::string url();
	void set_filename(const std::string& str);
	void set_url(const std::string& url);
	void set_progress(double downloaded, double total);
	void set_status(dlstatus dls);
	void set_kbps(double kbps);
	double kbps();
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
	dlstatus download_status;
	float cursize;
	float totalsize;
	double curkbps;
	unsigned long offs;
	pb_controller* ctrl;
};

} // namespace podboat

#endif /* PODBOAT_DOWNLOAD_H_ */

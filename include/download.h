#ifndef PODBEUTER_DOWNLOAD__H
#define PODBEUTER_DOWNLOAD__H

#include <string>

namespace podbeuter {

enum dlstatus_t { DL_QUEUED = 0, DL_DOWNLOADING, DL_CANCELLED, DL_DELETED, DL_FINISHED, DL_FAILED, DL_ALREADY_DOWNLOADED, DL_READY, DL_PLAYED };

class pb_controller;

class download {
	public:
		download(pb_controller * c = 0);
		~download();
		double percents_finished();
		const char * status_text();
		inline dlstatus_t status() const { return dlstatus; }
		const char * filename();
		const char * url();
		void set_filename(const std::string& str);
		void set_url(const std::string& url);
		void set_progress(double cur, double max);
		void set_status(dlstatus_t dls);
		void set_kbps(double kbps);
		double kbps();
		void set_offset(unsigned long offset);

		inline double current_size() const { return cursize + offs; }
		inline double total_size() const { return totalsize + offs; }

	private:
		std::string fn;
		std::string url_;
		dlstatus_t dlstatus;
		float cursize;
		float totalsize;
		double curkbps;
		unsigned long offs;
		pb_controller * ctrl;
};

}

#endif

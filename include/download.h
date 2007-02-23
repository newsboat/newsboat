#ifndef PODBEUTER_DOWNLOAD__H
#define PODBEUTER_DOWNLOAD__H

#include <string>

namespace podbeuter {

enum dlstatus_t { DL_QUEUED = 0, DL_DOWNLOADING, DL_CANCELLED, DL_DELETED, DL_FINISHED };

class download {
	public:
		download();
		~download();
		float percents_finished();
		const char * status_text();
		const char * filename();
		void set_filename(const std::string& str);
		void set_url(const std::string& url);

	private:
		std::string fn;
		std::string url;
		dlstatus_t dlstatus;
		float cursize;
		float totalsize;
};

}

#endif

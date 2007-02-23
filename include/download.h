#ifndef PODBEUTER_DOWNLOAD__H
#define PODBEUTER_DOWNLOAD__H

namespace podbeuter {

class download {
	public:
		download() { }
		~download() { }
		float percents_finished(){ return 0.0; }
		const char * status_text() { return ""; } // must return one of "queued", "downloading", "cancelled", "finished"
		const char * filename() { return ""; }
};

}

#endif

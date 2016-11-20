#include <poddlthread.h>
#include <curl/curl.h>
#include <iostream>
#include <logger.h>
#include <config.h>
#include <utils.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <unistd.h>
#include <libgen.h>

using namespace newsbeuter;

namespace podbeuter {

static size_t my_write_data(void *buffer, size_t size, size_t nmemb, void *userp);
static int progress_callback(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow);

poddlthread::poddlthread(download * dl_, newsbeuter::configcontainer * c) : dl(dl_), f(new std::ofstream()), bytecount(0), cfg(c) {
}

poddlthread::~poddlthread() {
}

void poddlthread::operator()() {
	run();
}

void poddlthread::run() {
	// are we resuming previous download?
	bool resumed_download = false;

	gettimeofday(&tv1, nullptr);
	++bytecount;

	CURL * easyhandle = curl_easy_init();
	utils::set_common_curl_options(easyhandle, cfg);

	curl_easy_setopt(easyhandle, CURLOPT_URL, dl->url().c_str());
	curl_easy_setopt(easyhandle, CURLOPT_TIMEOUT, 0);
	// set up write functions:
	curl_easy_setopt(easyhandle, CURLOPT_WRITEFUNCTION, my_write_data);
	curl_easy_setopt(easyhandle, CURLOPT_WRITEDATA, this);

	// set up progress notification:
	curl_easy_setopt(easyhandle, CURLOPT_NOPROGRESS, 0);
	curl_easy_setopt(easyhandle, CURLOPT_PROGRESSFUNCTION, progress_callback);
	curl_easy_setopt(easyhandle, CURLOPT_PROGRESSDATA, this);

	// set up max download speed
	int max_dl_speed = cfg->get_configvalue_as_int("max-download-speed");
	if (max_dl_speed > 0) {
		curl_easy_setopt(easyhandle, CURLOPT_MAX_RECV_SPEED_LARGE, (curl_off_t)(max_dl_speed * 1024));
	}

	struct stat sb;

	if (stat(dl->filename().c_str(), &sb) == -1) {
		LOG(level::INFO, "poddlthread::run: stat failed: starting normal download");
		std::string filename = dl->filename();
		std::vector<char> directory(filename.begin(), filename.end());
		utils::mkdir_parents(dirname(&directory[0]));
		f->open(dl->filename().c_str(), std::fstream::out);
		dl->set_offset(0);
		resumed_download = false;
	} else {
		LOG(level::INFO, "poddlthread::run: stat ok: starting download from %u", sb.st_size);
		curl_easy_setopt(easyhandle, CURLOPT_RESUME_FROM, sb.st_size);
		dl->set_offset(sb.st_size);
		f->open(dl->filename(), std::fstream::out | std::fstream::app);
		resumed_download = true;
	}

	if (f->is_open()) {

		dl->set_status(dlstatus::DOWNLOADING);

		CURLcode success = curl_easy_perform(easyhandle);

		f->close();

		LOG(level::INFO,"poddlthread::run: curl_easy_perform rc = %u (%s)", success, curl_easy_strerror(success));

		if (0 == success)
			dl->set_status(dlstatus::READY);
		else if (dl->status() != dlstatus::CANCELLED) {
			// attempt complete re-download
			if (resumed_download) {
				::unlink(dl->filename().c_str());
				this->run();
			} else {
				dl->set_status(dlstatus::FAILED);
				::unlink(dl->filename().c_str());
			}
		}
	} else {
		dl->set_status(dlstatus::FAILED);
	}

	curl_easy_cleanup(easyhandle);
}

static size_t my_write_data(void *buffer, size_t size, size_t nmemb, void *userp) {
	poddlthread * thread = static_cast<poddlthread *>(userp);
	return thread->write_data(buffer, size, nmemb);
}

static int progress_callback(void *clientp, double dltotal, double dlnow, double /* ultotal */, double /*ulnow*/) {
	poddlthread * thread = static_cast<poddlthread *>(clientp);
	return thread->progress(dlnow, dltotal);
}

size_t poddlthread::write_data(void * buffer, size_t size, size_t nmemb) {
	if (dl->status() == dlstatus::CANCELLED)
		return 0;
	f->write(static_cast<char *>(buffer), size * nmemb);
	bytecount += (size * nmemb);
	LOG(level::DEBUG, "poddlthread::write_data: bad = %u size = %u", f->bad(), size * nmemb);
	return f->bad() ? 0 : size * nmemb;
}

int poddlthread::progress(double dlnow, double dltotal) {
	if (dl->status() == dlstatus::CANCELLED)
		return -1;
	gettimeofday(&tv2, nullptr);
	double kbps = compute_kbps();
	if (kbps > 9999.99) {
		kbps = 0.0;
		gettimeofday(&tv1, nullptr);
		bytecount = 0;
	}
	dl->set_kbps(kbps);
	dl->set_progress(dlnow, dltotal);
	return 0;
}

double poddlthread::compute_kbps() {
	double t1 = tv1.tv_sec + (tv1.tv_usec/1000000.0);
	double t2 = tv2.tv_sec + (tv2.tv_usec/1000000.0);

	double result = (bytecount / (t2 - t1))/1024;

	return result;
}

}

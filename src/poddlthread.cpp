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

poddlthread::poddlthread(download * dl_, newsbeuter::configcontainer * c) : dl(dl_), f(new std::ofstream()), cfg(c) {
}

poddlthread::~poddlthread() {
	delete f;
}

void poddlthread::operator()() {
	gettimeofday(&tv1, NULL);
	++bytecount;

	CURL * easyhandle = curl_easy_init();
	utils::set_common_curl_options(easyhandle, cfg);

	curl_easy_setopt(easyhandle, CURLOPT_URL, dl->url());
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

	if (stat(dl->filename(), &sb) == -1) {
		LOG(LOG_INFO, "poddlthread::run: stat failed: starting normal download");
		mkdir_p(dl->filename());
		f->open(dl->filename(), std::fstream::out);
		dl->set_offset(0);
	} else {
		LOG(LOG_INFO, "poddlthread::run: stat ok: starting download from %u", sb.st_size);
		curl_easy_setopt(easyhandle, CURLOPT_RESUME_FROM, sb.st_size);
		dl->set_offset(sb.st_size);
		f->open(dl->filename(), std::fstream::out | std::fstream::app);
	}

	if (f->is_open()) {

		dl->set_status(DL_DOWNLOADING);

		CURLcode success = curl_easy_perform(easyhandle);

		f->close();

		LOG(LOG_INFO,"poddlthread::run: curl_easy_perform rc = %u (%s)", success, curl_easy_strerror(success));

		if (0 == success)
			dl->set_status(DL_READY);
		else if (dl->status() != DL_CANCELLED) {
			dl->set_status(DL_FAILED);
			::unlink(dl->filename());
		}
	} else {
		dl->set_status(DL_FAILED);
	}

	curl_easy_cleanup(easyhandle);
}

static size_t my_write_data(void *buffer, size_t size, size_t nmemb, void *userp) {
	poddlthread * thread = (poddlthread *)userp;
	return thread->write_data(buffer, size, nmemb);
}

static int progress_callback(void *clientp, double dltotal, double dlnow, double /* ultotal */, double /*ulnow*/) {
	poddlthread * thread = (poddlthread *)clientp;
	return thread->progress(dlnow, dltotal);
}

size_t poddlthread::write_data(void * buffer, size_t size, size_t nmemb) {
	if (dl->status() == DL_CANCELLED)
		return 0;
	f->write(static_cast<char *>(buffer), size * nmemb);
	bytecount += (size * nmemb);
	LOG(LOG_DEBUG, "poddlthread::write_data: bad = %u size = %u", f->bad(), size * nmemb);
	return f->bad() ? 0 : size * nmemb;
}

int poddlthread::progress(double dlnow, double dltotal) {
	if (dl->status() == DL_CANCELLED)
		return -1;
	gettimeofday(&tv2, NULL);
	double kbps = compute_kbps();
	if (kbps > 9999.99) {
		kbps = 0.0;
		gettimeofday(&tv1, NULL);
		bytecount = 0;
	}
	dl->set_kbps(kbps);
	dl->set_progress(dlnow, dltotal);
	return 0;
}

double poddlthread::compute_kbps() {
	double result = 0.0;

	double t1 = tv1.tv_sec + (tv1.tv_usec/(double)1000000);
	double t2 = tv2.tv_sec + (tv2.tv_usec/(double)1000000);

	result = (bytecount / (t2 - t1))/1024;

	return result;
}

void poddlthread::mkdir_p(const char * file) {
	char path[2048];
	snprintf(path, sizeof(path), "%s", file);
	for (char * x = path;*x != '\0';x++) {
		if (*x == '/') {
			*x = '\0';
			mkdir(path, 0755);
			*x = '/';
		}
	}
}

}

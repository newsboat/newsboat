#include <poddlthread.h>
#include <curl/curl.h>
#include <iostream>

namespace podbeuter {

static size_t my_write_data(void *buffer, size_t size, size_t nmemb, void *userp);
static int progress_callback(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow);

poddlthread::poddlthread(download * dl_) : dl(dl_) {
}

poddlthread::~poddlthread() {
}

void poddlthread::run() {
	gettimeofday(&tv1, NULL);
	++bytecount;

	CURL * easyhandle = curl_easy_init();
	curl_easy_setopt(easyhandle, CURLOPT_URL, dl->url());
	// set up write functions:
	curl_easy_setopt(easyhandle, CURLOPT_WRITEFUNCTION, my_write_data);
	curl_easy_setopt(easyhandle, CURLOPT_WRITEDATA, this);

	// set up progress notification:
	curl_easy_setopt(easyhandle, CURLOPT_NOPROGRESS, 0);
	curl_easy_setopt(easyhandle, CURLOPT_PROGRESSFUNCTION, progress_callback);
	curl_easy_setopt(easyhandle, CURLOPT_PROGRESSDATA, this);

	f.open(dl->filename(), std::fstream::out);

	if (f.is_open()) {

		dl->set_status(DL_DOWNLOADING);

		CURLcode success = curl_easy_perform(easyhandle);

		f.close();

		if (0 == success)
			dl->set_status(DL_FINISHED);
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
	// std::cerr << "my_write_data(...," << size << "," << nmemb << ",...) called" << std::endl;
	return thread->write_data(buffer, size, nmemb);
}

static int progress_callback(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow) {
	poddlthread * thread = (poddlthread *)clientp;
	// std::cerr << "progress_callback(...," << dltotal << "," << dlnow << ",...) called" << std::endl;
	return thread->progress(dlnow, dltotal);
}

size_t poddlthread::write_data(void * buffer, size_t size, size_t nmemb) {
	if (dl->status() == DL_CANCELLED)
		return 0;
	f.write(static_cast<char *>(buffer), size * nmemb);
	bytecount += (size * nmemb);
	return f.bad() ? 0 : size * nmemb;
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

}

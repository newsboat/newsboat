#include "poddlthread.h"

#include <curl/curl.h>
#include <iostream>
#include <libgen.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <unistd.h>

#include "config.h"
#include "logger.h"
#include "utils.h"

using namespace newsboat;

namespace podboat {

static size_t
my_write_data(void* buffer, size_t size, size_t nmemb, void* userp);
static int progress_callback(void* clientp,
	double dltotal,
	double dlnow,
	double ultotal,
	double ulnow);

PodDlThread::PodDlThread(Download* dl_, newsboat::ConfigContainer* c)
	: dl(dl_)
	, f(new std::ofstream())
	, bytecount(0)
	, cfg(c)
{
	static const timeval zero = {0, 0};
	tv1 = zero;
	tv2 = zero;
}

PodDlThread::~PodDlThread() {}

void PodDlThread::operator()()
{
	run();
}

void PodDlThread::run()
{
	// are we resuming previous download?
	bool resumed_download = false;

	gettimeofday(&tv1, nullptr);
	++bytecount;

	CURL* easyhandle = curl_easy_init();
	Utils::set_common_curl_options(easyhandle, cfg);

	curl_easy_setopt(easyhandle, CURLOPT_URL, dl->url().c_str());
	curl_easy_setopt(easyhandle, CURLOPT_TIMEOUT, 0);
	// set up write functions:
	curl_easy_setopt(easyhandle, CURLOPT_WRITEFUNCTION, my_write_data);
	curl_easy_setopt(easyhandle, CURLOPT_WRITEDATA, this);

	// set up progress notification:
	curl_easy_setopt(easyhandle, CURLOPT_NOPROGRESS, 0);
	curl_easy_setopt(
		easyhandle, CURLOPT_PROGRESSFUNCTION, progress_callback);
	curl_easy_setopt(easyhandle, CURLOPT_PROGRESSDATA, this);

	// set up max download speed
	int max_dl_speed = cfg->get_configvalue_as_int("max-download-speed");
	if (max_dl_speed > 0) {
		curl_easy_setopt(easyhandle,
			CURLOPT_MAX_RECV_SPEED_LARGE,
			(curl_off_t)(max_dl_speed * 1024));
	}

	struct stat sb;
	std::string filename =
		dl->filename() + newsboat::ConfigContainer::PARTIAL_FILE_SUFFIX;

	if (stat(filename.c_str(), &sb) == -1) {
		LOG(Level::INFO,
			"PodDlThread::run: stat failed: starting normal "
			"download");

		// Have to copy the string into a vector in order to be able to
		// get a char* pointer. std::string::c_str() won't do because it
		// returns const char*, whereas ::dirname() needs non-const.
		std::vector<char> directory(filename.begin(), filename.end());
		Utils::mkdir_parents(dirname(&directory[0]));

		f->open(filename, std::fstream::out);
		dl->set_offset(0);
		resumed_download = false;
	} else {
		LOG(Level::INFO,
			"PodDlThread::run: stat ok: starting download from %u",
			sb.st_size);
		curl_easy_setopt(easyhandle, CURLOPT_RESUME_FROM, sb.st_size);
		dl->set_offset(sb.st_size);
		f->open(filename, std::fstream::out | std::fstream::app);
		resumed_download = true;
	}

	if (f->is_open()) {
		dl->set_status(DlStatus::DOWNLOADING);

		CURLcode success = curl_easy_perform(easyhandle);

		f->close();

		LOG(Level::INFO,
			"PodDlThread::run: curl_easy_perform rc = %u (%s)",
			success,
			curl_easy_strerror(success));

		if (0 == success) {
			LOG(Level::DEBUG,
				"PodDlThread::run: download complete, deleting "
				"temporary suffix");
			rename(filename.c_str(), dl->filename().c_str());
			dl->set_status(DlStatus::READY);
		} else if (dl->status() != DlStatus::CANCELLED) {
			// attempt complete re-download
			if (resumed_download) {
				::unlink(filename.c_str());
				this->run();
			} else {
				dl->set_status(DlStatus::FAILED);
				::unlink(filename.c_str());
			}
		}
	} else {
		dl->set_status(DlStatus::FAILED);
	}

	curl_easy_cleanup(easyhandle);
}

static size_t
my_write_data(void* buffer, size_t size, size_t nmemb, void* userp)
{
	PodDlThread* thread = static_cast<PodDlThread*>(userp);
	return thread->write_data(buffer, size, nmemb);
}

static int progress_callback(void* clientp,
	double dltotal,
	double dlnow,
	double /* ultotal */,
	double /*ulnow*/)
{
	PodDlThread* thread = static_cast<PodDlThread*>(clientp);
	return thread->progress(dlnow, dltotal);
}

size_t PodDlThread::write_data(void* buffer, size_t size, size_t nmemb)
{
	if (dl->status() == DlStatus::CANCELLED)
		return 0;
	f->write(static_cast<char*>(buffer), size * nmemb);
	bytecount += (size * nmemb);
	LOG(Level::DEBUG,
		"PodDlThread::write_data: bad = %u size = %u",
		f->bad(),
		size * nmemb);
	return f->bad() ? 0 : size * nmemb;
}

int PodDlThread::progress(double dlnow, double dltotal)
{
	if (dl->status() == DlStatus::CANCELLED)
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

double PodDlThread::compute_kbps()
{
	double t1 = tv1.tv_sec + (tv1.tv_usec / 1000000.0);
	double t2 = tv2.tv_sec + (tv2.tv_usec / 1000000.0);

	double result = (bytecount / (t2 - t1)) / 1024;

	return result;
}

} // namespace podboat

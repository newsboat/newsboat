#define ENABLE_IMPLICIT_FILEPATH_CONVERSIONS

#include "poddlthread.h"

#include <cstring>
#include <cinttypes>
#include <curl/curl.h>
#include <libgen.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <unistd.h>

#include "curlhandle.h"
#include "logger.h"
#include "utils.h"

using namespace newsboat;

namespace podboat {

static size_t my_write_data(void* buffer, size_t size, size_t nmemb,
	void* userp);
static int progress_callback(void* clientp,
	curl_off_t dltotal,
	curl_off_t dlnow,
	curl_off_t ultotal,
	curl_off_t ulnow);

PodDlThread::PodDlThread(Download* dl_, newsboat::ConfigContainer& c)
	: dl(dl_)
	, f(new std::ofstream())
	, bytecount(0)
	, cfg(c)
{
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

	tv1 = std::chrono::steady_clock::now();
	++bytecount;

	CurlHandle handle;
	utils::set_common_curl_options(handle, cfg);

	curl_easy_setopt(handle.ptr(), CURLOPT_URL, dl->url().c_str());
	curl_easy_setopt(handle.ptr(), CURLOPT_TIMEOUT, 0);
	// set up write functions:
	curl_easy_setopt(handle.ptr(), CURLOPT_WRITEFUNCTION, my_write_data);
	curl_easy_setopt(handle.ptr(), CURLOPT_WRITEDATA, this);

	// set up progress notification:
	curl_easy_setopt(handle.ptr(), CURLOPT_NOPROGRESS, 0);
	curl_easy_setopt(
		handle.ptr(), CURLOPT_XFERINFOFUNCTION, progress_callback);
	curl_easy_setopt(handle.ptr(), CURLOPT_XFERINFODATA, this);

	// set up max download speed
	int max_dl_speed = cfg.get_configvalue_as_int("max-download-speed");
	if (max_dl_speed > 0) {
		curl_easy_setopt(handle.ptr(),
			CURLOPT_MAX_RECV_SPEED_LARGE,
			(curl_off_t)(max_dl_speed * 1024));
	}

	struct stat sb;
	Filepath filename =
		dl->filename().join(newsboat::ConfigContainer::PARTIAL_FILE_SUFFIX);

	if (stat(filename.to_locale_string().c_str(), &sb) == -1) {
		LOG(Level::INFO,
			"PodDlThread::run: stat failed: starting normal "
			"download");

		// Have to copy the string into a vector in order to be able to
		// get a char* pointer. std::string::c_str() won't do because it
		// returns const char*, whereas ::dirname() needs non-const.
		std::vector<char> directory(filename.to_locale_string().begin(),
			filename.to_locale_string().end());
		directory.push_back('\0');
		utils::mkdir_parents(dirname(&directory[0]));

		f->open(filename, std::fstream::out);
		dl->set_offset(0);
		resumed_download = false;
	} else {
		LOG(Level::INFO,
			"PodDlThread::run: stat ok: starting download from %" PRIi64,
			// That field is `long int`, which is at least 32 bits. On x86_64,
			// it's 64 bits. Thus, this cast is either a no-op, or an up-cast
			// which are always safe.
			static_cast<int64_t>(sb.st_size));
		curl_easy_setopt(handle.ptr(), CURLOPT_RESUME_FROM, sb.st_size);
		dl->set_offset(sb.st_size);
		f->open(filename, std::fstream::out | std::fstream::app);
		resumed_download = true;
	}

	if (f->is_open()) {
		dl->set_status(DlStatus::DOWNLOADING);

		CURLcode success = curl_easy_perform(handle.ptr());

		f->close();

		LOG(Level::INFO,
			"PodDlThread::run: curl_easy_perform rc = %u (%s)",
			success,
			curl_easy_strerror(success));

		if (0 == success) {
			LOG(Level::DEBUG,
				"PodDlThread::run: download complete, deleting "
				"temporary suffix");
			if (rename(filename.to_locale_string().c_str(),
					dl->filename().to_locale_string().c_str()) == 0) {
				dl->set_status(DlStatus::READY);
			} else {
				dl->set_status(DlStatus::RENAME_FAILED, strerror(errno));
			}
		} else if (dl->status() != DlStatus::CANCELLED) {
			// attempt complete re-download
			if (resumed_download) {
				::unlink(filename.to_locale_string().c_str());
				this->run();
			} else {
				dl->set_status(DlStatus::FAILED, curl_easy_strerror(success));
				::unlink(filename.to_locale_string().c_str());
			}
		}
	} else {
		dl->set_status(DlStatus::FAILED, strerror(errno));
	}
}

static size_t my_write_data(void* buffer, size_t size, size_t nmemb,
	void* userp)
{
	PodDlThread* thread = static_cast<PodDlThread*>(userp);
	return thread->write_data(buffer, size, nmemb);
}

static int progress_callback(void* clientp,
	curl_off_t dltotal,
	curl_off_t dlnow,
	curl_off_t /* ultotal */,
	curl_off_t /*ulnow*/)
{
	PodDlThread* thread = static_cast<PodDlThread*>(clientp);
	return thread->progress(dlnow, dltotal);
}

size_t PodDlThread::write_data(void* buffer, size_t size, size_t nmemb)
{
	if (dl->status() == DlStatus::CANCELLED) {
		return 0;
	}
	f->write(static_cast<char*>(buffer), size * nmemb);
	bytecount += (size * nmemb);
	LOG(Level::DEBUG,
		"PodDlThread::write_data: bad = %u size = %" PRIu64,
		f->bad(),
		static_cast<uint64_t>(size * nmemb));
	return f->bad() ? 0 : size * nmemb;
}

int PodDlThread::progress(double dlnow, double dltotal)
{
	if (dl->status() == DlStatus::CANCELLED) {
		return -1;
	}
	tv2 = std::chrono::steady_clock::now();
	const double kbps = compute_kbps();
	dl->set_kbps(kbps);
	dl->set_progress(dlnow, dltotal);
	return 0;
}

double PodDlThread::compute_kbps()
{
	using fpseconds = std::chrono::duration<double>;

	const double elapsed = std::chrono::duration_cast<fpseconds>(tv2 - tv1).count();
	const double result = (bytecount / elapsed) / 1024;

	return result;
}

} // namespace podboat

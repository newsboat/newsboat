#ifndef NEWSBOAT_CURLHANDLE_H_
#define NEWSBOAT_CURLHANDLE_H_

#include <curl/curl.h>
#include <stdexcept>

namespace Newsboat {

// wrapped curl handle for exception safety and so on
// see also: https://github.com/gsauthof/ccurl
class CurlHandle {
private:
	CURL* h;
	CurlHandle(const CurlHandle&) = delete;
	CurlHandle& operator=(const CurlHandle&) = delete;

	void cleanup()
	{
		if (h != nullptr) {
			curl_easy_cleanup(h);
		}
	}

public:
	CurlHandle()
		: h(curl_easy_init())
	{
		if (!h) {
			throw std::runtime_error("Can't obtain curl handle");
		}
	}
	~CurlHandle()
	{
		cleanup();
	}
	CurlHandle(CurlHandle&& other)
		: h(other.h)
	{
		other.h = nullptr;
	}
	CurlHandle& operator=(CurlHandle&& other)
	{
		cleanup();
		h = other.h;
		other.h = nullptr;
		return *this;
	}

	CURL* ptr()
	{
		return h;
	}
};

} // namespace Newsboat

#endif /* NEWSBOAT_CURLHANDLE_H_ */


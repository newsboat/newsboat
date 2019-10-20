#ifndef NEWSBOAT_CURLHANDLE_H_
#define NEWSBOAT_CURLHANDLE_H_

#include <curl/curl.h>
#include <stdexcept>

namespace newsboat {

// wrapped curl handle for exception safety and so on
// see also: https://github.com/gsauthof/ccurl
class CurlHandle {
private:
	CURL* h;
	CurlHandle(const CurlHandle&);
	CurlHandle& operator=(const CurlHandle&);

public:
	CurlHandle()
		: h(0)
	{
		h = curl_easy_init();
		if (!h) {
			throw std::runtime_error("Can't obtain curl handle");
		}
	}
	~CurlHandle()
	{
		curl_easy_cleanup(h);
	}
	CURL* ptr()
	{
		return h;
	}
};

} // namespace newsboat

#endif /* NEWSBOAT_CURLHANDLE_H_ */


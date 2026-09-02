#ifndef NEWSBOAT_CURLDATARECEIVER_H_
#define NEWSBOAT_CURLDATARECEIVER_H_

#include <cstddef>
#include <memory>
#include <string>

#include "curlhandle.h"

namespace newsboat {

class CurlDataReceiver {
public:
	/// A max_data_size of zero leaves the receiver unbounded.
	static std::unique_ptr<CurlDataReceiver> register_data_handler(
		CurlHandle& curlHandle, std::size_t max_data_size = 0);

	const std::string& get_data() const;
	virtual ~CurlDataReceiver();

protected:
	explicit CurlDataReceiver(CurlHandle& curlHandle, std::size_t max_data_size);
	size_t handle_data(const char* data, size_t data_size);

	CurlDataReceiver(const CurlDataReceiver&) = delete;
	CurlDataReceiver(CurlDataReceiver&&) = delete;
	CurlDataReceiver& operator=(const CurlDataReceiver&) = delete;
	CurlDataReceiver& operator=(CurlDataReceiver&&) = delete;

private:
	static size_t write_callback(char* buffer, size_t size, size_t nmemb, void* receiver);

	CurlHandle& curl_handle;
	const std::size_t max_data_size;
	std::string accumulated_data;
};

} // namespace newsboat

#endif /* NEWSBOAT_CURLDATARECEIVER_H_ */

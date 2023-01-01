#ifndef NEWSBOAT_CURLDATARECEIVER_H_
#define NEWSBOAT_CURLDATARECEIVER_H_

#include <cstdint>
#include <memory>
#include <string>

#include "curlhandle.h"

namespace newsboat {

class CurlDataReceiver {
public:
	// Registration is only valid as long as the pointer is alive
	[[nodiscard]]
	static std::unique_ptr<CurlDataReceiver> register_data_handler(CurlHandle& curlHandle);

	const std::string& get_data() const;
	virtual ~CurlDataReceiver();

protected:
	explicit CurlDataReceiver(CurlHandle& curlHandle);
	void handle_data(const std::string& data);

	CurlDataReceiver(const CurlDataReceiver&) = delete;
	CurlDataReceiver(CurlDataReceiver&&) = delete;
	CurlDataReceiver& operator=(const CurlDataReceiver&) = delete;
	CurlDataReceiver& operator=(CurlDataReceiver&&) = delete;

private:
	static size_t write_callback(char* buffer, size_t size, size_t nmemb, void* receiver);

	CurlHandle& mCurlHandle;
	std::string mData;
};

} // namespace newsboat

#endif /* NEWSBOAT_CURLDATARECEIVER_H_ */

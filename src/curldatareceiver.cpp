#include "curldatareceiver.h"

#include <limits>

namespace newsboat {

std::unique_ptr<newsboat::CurlDataReceiver> CurlDataReceiver::register_data_handler(
	CurlHandle& curlHandle, std::size_t max_data_size)
{
	return std::unique_ptr<CurlDataReceiver>(
		new CurlDataReceiver(curlHandle, max_data_size));
}

const std::string& CurlDataReceiver::get_data() const
{
	return accumulated_data;
}

CurlDataReceiver::CurlDataReceiver(CurlHandle& curlHandle,
	std::size_t max_data_size_)
	: curl_handle(curlHandle)
	, max_data_size(max_data_size_)
{
	curl_easy_setopt(curl_handle.ptr(), CURLOPT_WRITEDATA, this);
	curl_easy_setopt(curl_handle.ptr(), CURLOPT_WRITEFUNCTION,
		&CurlDataReceiver::write_callback);
}

CurlDataReceiver::~CurlDataReceiver()
{
	curl_easy_setopt(curl_handle.ptr(), CURLOPT_WRITEDATA, nullptr);
	curl_easy_setopt(curl_handle.ptr(), CURLOPT_WRITEFUNCTION, nullptr);
}

size_t CurlDataReceiver::write_callback(char* buffer, size_t size, size_t nmemb,
	void* receiver)
{
	auto data_receiver = static_cast<CurlDataReceiver*>(receiver);
	if (nmemb != 0 && size > std::numeric_limits<size_t>::max() / nmemb) {
		return 0;
	}
	return data_receiver->handle_data(buffer, size * nmemb);
}

size_t CurlDataReceiver::handle_data(const char* data, size_t data_size)
{
	if (max_data_size != 0
		&& (data_size > max_data_size
			|| accumulated_data.size() > max_data_size - data_size)) {
		return 0;
	}

	accumulated_data.append(data, data_size);
	return data_size;
}

} // namespace newsboat

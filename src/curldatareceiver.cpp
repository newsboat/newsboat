#include "curldatareceiver.h"

namespace Newsboat {

std::unique_ptr<Newsboat::CurlDataReceiver> CurlDataReceiver::register_data_handler(
	CurlHandle& curlHandle)
{
	return std::unique_ptr<CurlDataReceiver>(new CurlDataReceiver(curlHandle));
}

const std::string& CurlDataReceiver::get_data() const
{
	return accumulated_data;
}

CurlDataReceiver::CurlDataReceiver(CurlHandle& curlHandle)
	: curl_handle(curlHandle)
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
	const auto data = std::string(buffer, size * nmemb);

	data_receiver->handle_data(data);

	return size * nmemb;
}

void CurlDataReceiver::handle_data(const std::string& data)
{
	accumulated_data += data;
}

} // namespace Newsboat

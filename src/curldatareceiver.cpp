#include "curldatareceiver.h"

namespace newsboat {

std::unique_ptr<newsboat::CurlDataReceiver> CurlDataReceiver::register_data_handler(
	CurlHandle& curlHandle)
{
	return std::unique_ptr<CurlDataReceiver>(new CurlDataReceiver(curlHandle));
}

const std::string& CurlDataReceiver::get_data() const
{
	return mData;
}

CurlDataReceiver::CurlDataReceiver(CurlHandle& curlHandle)
	: mCurlHandle(curlHandle)
{
	curl_easy_setopt(mCurlHandle.ptr(), CURLOPT_WRITEDATA, this);
	curl_easy_setopt(mCurlHandle.ptr(), CURLOPT_WRITEFUNCTION,
		&CurlDataReceiver::write_callback);
}

CurlDataReceiver::~CurlDataReceiver()
{
	curl_easy_setopt(mCurlHandle.ptr(), CURLOPT_WRITEDATA, nullptr);
	curl_easy_setopt(mCurlHandle.ptr(), CURLOPT_WRITEFUNCTION, nullptr);
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
	mData += data;
}

} // namespace newsboat

#include "curlheadercontainer.h"

#include <curl/curl.h>

namespace newsboat {

std::unique_ptr<CurlHeaderContainer> CurlHeaderContainer::register_header_handler(
	CurlHandle& curlHandle)
{
	return std::unique_ptr<CurlHeaderContainer>(new CurlHeaderContainer(curlHandle));
}

const std::vector<std::string>& CurlHeaderContainer::get_header_lines() const
{
	return mHeaderLines;
}

CurlHeaderContainer::CurlHeaderContainer(CurlHandle& curlHandle)
	: mCurlHandle(curlHandle)
{
	curl_easy_setopt(mCurlHandle.ptr(), CURLOPT_HEADERDATA, this);
	curl_easy_setopt(mCurlHandle.ptr(), CURLOPT_HEADERFUNCTION,
		&CurlHeaderContainer::handle_headers);
}

CurlHeaderContainer::~CurlHeaderContainer()
{
	curl_easy_setopt(mCurlHandle.ptr(), CURLOPT_HEADERDATA, nullptr);
	curl_easy_setopt(mCurlHandle.ptr(), CURLOPT_HEADERFUNCTION, nullptr);
}

size_t CurlHeaderContainer::handle_headers(char* buffer, size_t size, size_t nitems,
	void* data)
{
	auto header_container = static_cast<CurlHeaderContainer*>(data);
	const auto header_line = std::string(buffer, nitems * size);

	header_container->handle_header(header_line);

	return nitems * size;
}

void CurlHeaderContainer::handle_header(const std::string& line)
{
	if (line.find("HTTP/") == 0) {
		// Reset headers if a new response is detected (there might be multiple responses per request in case of a redirect)
		mHeaderLines.clear();
	}

	mHeaderLines.push_back(line);
}

} // namespace newsboat

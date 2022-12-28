#ifndef NEWSBOAT_CURLHEADERCONTAINER_H_
#define NEWSBOAT_CURLHEADERCONTAINER_H_

#include <memory>
#include <string>
#include <vector>

#include "curlhandle.h"

namespace newsboat {

class CurlHeaderContainer {
public:
	[[nodiscard("Registration is only valid as long as the pointer is alive")]]
	static std::unique_ptr<CurlHeaderContainer> register_header_handler(
		CurlHandle& curlHandle);

	const std::vector<std::string>& get_header_lines() const;

private:
	explicit CurlHeaderContainer(CurlHandle& curlHandle);
public:
	~CurlHeaderContainer();

private:
	CurlHeaderContainer(const CurlHeaderContainer&) = delete;
	CurlHeaderContainer(CurlHeaderContainer&&) = delete;
	CurlHeaderContainer& operator=(const CurlHeaderContainer&) = delete;
	CurlHeaderContainer& operator=(CurlHeaderContainer&&) = delete;

	static size_t handle_headers(char* buffer, size_t size, size_t nitems, void* data);
	void handle_header(const std::string& line);

	CurlHandle& mCurlHandle;
	std::vector<std::string> mHeaderLines;
};

} // namespace newsboat

#endif /* NEWSBOAT_CURLHEADERCONTAINER_H_ */

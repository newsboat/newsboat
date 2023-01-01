#ifndef NEWSBOAT_CURLHEADERCONTAINER_H_
#define NEWSBOAT_CURLHEADERCONTAINER_H_

#include <memory>
#include <string>
#include <vector>

#include "curlhandle.h"

namespace newsboat {

class CurlHeaderContainer {
public:
	// Registration is only valid as long as the pointer is alive
	[[nodiscard]]
	static std::unique_ptr<CurlHeaderContainer> register_header_handler(
		CurlHandle& curlHandle);

	const std::vector<std::string>& get_header_lines() const;
	std::vector<std::string> get_header_lines(const std::string& key) const;
	virtual ~CurlHeaderContainer();

protected:
	explicit CurlHeaderContainer(CurlHandle& curlHandle);
	void handle_header(const std::string& line);

	CurlHeaderContainer(const CurlHeaderContainer&) = delete;
	CurlHeaderContainer(CurlHeaderContainer&&) = delete;
	CurlHeaderContainer& operator=(const CurlHeaderContainer&) = delete;
	CurlHeaderContainer& operator=(CurlHeaderContainer&&) = delete;

private:
	static size_t handle_headers(char* buffer, size_t size, size_t nitems, void* data);

	CurlHandle& mCurlHandle;
	std::vector<std::string> mHeaderLines;
};

} // namespace newsboat

#endif /* NEWSBOAT_CURLHEADERCONTAINER_H_ */
